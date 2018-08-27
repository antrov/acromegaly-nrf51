#include "controller.h"
#include "app_error.h"
#include "app_pwm.h"
#include "aufzug_config.h"
#include "boards.h"
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "tick_generator.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#define GPIO_MOTOR_ENABLED 28
#define GPIO_MOTOR_UP 16
#define GPIO_MOTOR_DOWN 15
#define GPIO_TICK_INPUT 29
#define GPIO_TICK_OUTPUT 01 /* Tick PWM generator, fo testing purpoese */

#define NIL_POSITION -1 /* Marks target as unset */

#define DIRECTION_DEBUG(direction) direction == MOVE_DIRECTION_UP ? 1 : (direction == MOVE_DIRECTION_DOWN ? -1 : 0)
#define controller_call_cb() \
    if (m_cb)                \
    m_cb(&m_state)

static controller_cb_t m_cb; /* Current state of controller */
static controller_state_t m_state; /* COntroller state callback method. Optional */

static bool isInit = false; /* For single init of controller */

void update_current_position(int position);
void controller_move(uint8_t direction);
void controller_switch(uint8_t switch_state);
void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

void update_current_position(int position)
{
    m_state.position = position;
    controller_call_cb();
}

void controller_init(int position)
{
    if (isInit)
        return;

    m_state.movement = MOVE_DIRECTION_UP;
    // m_state.movement = MOVE_DIRECTION_NONE;
    m_state.position = position;
    m_state.target = NIL_POSITION;
    m_state.global_switch = SWITCH_OFF;

    ret_code_t err_code;

    nrf_drv_gpiote_init();

    /* TICK Input pins config */
    nrf_drv_gpiote_in_config_t tick_in_config = GPIOTE_CONFIG_IN_SENSE_LOTOHI(true);
    tick_in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(GPIO_TICK_INPUT, &tick_in_config, in_pin_handler);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(GPIO_TICK_INPUT, true);

    /* Output Pins config */
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

    err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_UP, &out_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_DOWN, &out_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_ENABLED, &out_config);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_PRINTF("Ctrl init pos %d\r\n", position);

/* Tick generator init */
#if USE_TICK_GENERATOR
    tick_generator_init(GPIO_TICK_OUTPUT);
    NRF_LOG_PRINTF("Init tick generator");
#endif

    isInit = true;
}

#if USE_TICK_GENERATOR
void update_tick_generator()
{
    if (m_state.movement != MOVE_DIRECTION_NONE) {
        tick_generator_start();
        NRF_LOG_PRINTF("Startted tick generator\n");
    } else {
        tick_generator_stop();
        NRF_LOG_PRINTF("Stopped tick generator\n");
    }
}
#endif

void controller_register_cb(controller_cb_t cb)
{
    m_cb = cb;
    controller_call_cb();
}

void controller_move(uint8_t direction)
{
    nrf_drv_gpiote_out_clear(GPIO_MOTOR_UP);
    nrf_drv_gpiote_out_clear(GPIO_MOTOR_DOWN);
    m_state.movement = MOVE_DIRECTION_NONE;

    uint8_t motor = 0x00;

    switch (direction) {
    case MOVE_DIRECTION_DOWN:
        motor = GPIO_MOTOR_DOWN;
        break;
    case MOVE_DIRECTION_UP:
        motor = GPIO_MOTOR_UP;
        break;
    case MOVE_DIRECTION_NONE:
    default:
        break;
    }

    if (motor == 0x00) {
        NRF_LOG_PRINTF("Ctrl dir did not change. Stopping\r\n");
        controller_switch(SWITCH_OFF);
    } else {
        NRF_LOG_PRINTF("Ctrl dir changed to %d\r\n", DIRECTION_DEBUG(direction));

        m_state.movement = direction;
        nrf_drv_gpiote_out_set(motor);
        controller_switch(SWITCH_ON);
    }

#if USE_TICK_GENERATOR
    update_tick_generator();
#endif

    controller_call_cb();
}

void controller_stop()
{
    controller_move(MOVE_DIRECTION_NONE);
}

void controller_target_position_set(int16_t target)
{
    if (target == NIL_POSITION) {
        m_state.target = NIL_POSITION;
        controller_stop();
        return;
    }

    if (target == m_state.position || target == m_state.target) {
        return;
    }

    NRF_LOG_PRINTF("Set target to %d (%d current)\r\n", target, m_state.position);

    m_state.target = target;

    if (target < m_state.position) {
        controller_move(MOVE_DIRECTION_DOWN);
    } else if (target > m_state.position) {
        controller_move(MOVE_DIRECTION_UP);
    } else
        return;

    controller_call_cb();
}

void controller_switch(uint8_t switch_state)
{
    NRF_LOG_PRINTF("Ctrl sw to %d\r\n", switch_state);

    if (switch_state == SWITCH_OFF) {
        nrf_drv_gpiote_out_clear(GPIO_MOTOR_ENABLED);
    } else if (switch_state == SWITCH_ON) {
        nrf_drv_gpiote_out_set(GPIO_MOTOR_ENABLED);
    } else
        return;

    m_state.global_switch = switch_state;
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    NRF_LOG_PRINTF("Tick at %d; targ %d; dir: %d\r\n", m_state.position, m_state.target, DIRECTION_DEBUG(m_state.movement));

    bool stop = m_state.target != NIL_POSITION;

    switch (m_state.movement) {
    case MOVE_DIRECTION_DOWN:
        update_current_position(m_state.position - 1);
        stop &= m_state.position <= m_state.target;
        break;
    case MOVE_DIRECTION_UP:
        update_current_position(m_state.position + 1);
        stop &= m_state.position >= m_state.target;
        break;
    case MOVE_DIRECTION_NONE:
        stop = true;
        return;
    default:
        break;
    }

    NRF_LOG_PRINTF("Pos -> %d\r\n", m_state.position);

    if (stop) {
        controller_stop();
        m_state.target = NIL_POSITION;
        controller_call_cb();
    }
}
