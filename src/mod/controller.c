#include "controller.h"
#include "app_error.h"
#include "app_pwm.h"
#include "app_timer.h"
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

/*===========================================================================*/
/* Controller local definitions.                                             */
/*===========================================================================*/

#define GPIO_MOTOR_ENABLED_PIN 28
#define GPIO_MOTOR_UP_PIN 16
#define GPIO_MOTOR_DOWN_PIN 15
#define GPIO_TICK_INPUT 29
#define GPIO_TICK_OUTPUT 01 /* Tick PWM generator, fo testing purpoese */

#define NIL_POSITION -1 /* Marks target as unset */

#define DIRECTION_DEBUG(direction) direction == MOVE_DIRECTION_UP ? "UP" : (direction == MOVE_DIRECTION_DOWN ? "DOWN" : "NONE")
#define controller_call_cb() \
    if (m_cb)                \
    m_cb(&m_state)

#define APP_CTRL_TIMER_INTERVAL_MS 150
#define APP_CTRL_TIMER_OP_QUEUE_SIZE 1 /**< Size of timer operation queues. */
#define APP_CTRL_TIMER_PRESCALER 0 /**< Value of the RTC1 PRESCALER register. */
#define APP_CTRL_TIMER_INTERVAL APP_TIMER_TICKS(APP_CTRL_TIMER_INTERVAL_MS, APP_CTRL_TIMER_PRESCALER) // 1000 ms intervals

#define CTR_TIMER_TICKS_STOP_THRESHOLD 1200 / APP_CTRL_TIMER_INTERVAL_MS /* How many ticks without position change is required to decide that movement has stopped */

/*===========================================================================*/
/* Controller exported variables.                                            */
/*===========================================================================*/

/*===========================================================================*/
/* Controller local variables and types.                                     */
/*===========================================================================*/

APP_TIMER_DEF(m_app_ctrl_timer_id);

static controller_cb_t m_cb; /* Current state of controller */
static controller_state_t m_state; /* COntroller state callback method. Optional */

uint32_t m_previous_inpin_state = 0;
int16_t m_previous_position = 0; /* Used to detect a premature end of movement */
uint8_t m_inert_movement = MOVE_DIRECTION_NONE;
uint32_t m_idle_counter = 0;

/*===========================================================================*/
/* Controller local functions.                                               */
/*===========================================================================*/

void set_reset(uint8_t reset);

void set_movement_dir(uint8_t direction)
{
    NRF_LOG_PRINTF("Ctrl dir set to %s\r\n", DIRECTION_DEBUG(direction));
    m_state.movement = direction;

    nrf_drv_gpiote_out_clear(GPIO_MOTOR_UP_PIN);
    nrf_drv_gpiote_out_clear(GPIO_MOTOR_DOWN_PIN);
    nrf_drv_gpiote_out_clear(GPIO_MOTOR_ENABLED_PIN);

    uint8_t motor = 0x00;

    switch (direction) {
    case MOVE_DIRECTION_DOWN:
        motor = GPIO_MOTOR_DOWN_PIN;
        break;
    case MOVE_DIRECTION_UP:
        motor = GPIO_MOTOR_UP_PIN;
        break;
    case MOVE_DIRECTION_NONE:
    default:
        break;
    }

    NRF_LOG_PRINTF("Motor is %d\r\n", motor);

    if (motor != 0x00) {
        m_inert_movement = direction;

        nrf_drv_gpiote_out_set(motor);
        nrf_drv_gpiote_out_set(GPIO_MOTOR_ENABLED_PIN);

        app_timer_start(m_app_ctrl_timer_id, APP_CTRL_TIMER_INTERVAL, NULL);
    }

#if USE_TICK_GENERATOR
    update_tick_generator();
#endif

    controller_call_cb();
}

void update_position()
{
    bool stop = m_state.target != NIL_POSITION;

    switch (m_inert_movement) {
    case MOVE_DIRECTION_DOWN:
        m_state.position--;
        stop &= m_state.position <= m_state.target;
        break;
    case MOVE_DIRECTION_UP:
        m_state.position++;
        stop &= m_state.position >= m_state.target;
        break;
    case MOVE_DIRECTION_NONE:
        stop = true;
        return;
    default:
        break;
    }

    if (stop) {
        controller_stop();
    }
}

void set_target_pos(int16_t target)
{
    NRF_LOG_PRINTF("%sSet target to %d%s\r\n", NRF_LOG_COLOR_GREEN, target, NRF_LOG_COLOR_DEFAULT);

    m_state.target = target;

    if (target == NIL_POSITION) {
        set_movement_dir(MOVE_DIRECTION_NONE);
    } else if (target < m_state.position) {
        set_movement_dir(MOVE_DIRECTION_DOWN);
    } else if (target > m_state.position) {
        set_movement_dir(MOVE_DIRECTION_UP);
    }
}

void sanitize_position()
{
    if (m_state.position > TICKS_UPPER_LIMIT) {
        m_state.position = TICKS_UPPER_LIMIT;
    } else if (m_state.position < TICK_LOWER_LIMIT) {
        m_state.position = TICK_LOWER_LIMIT;
    }
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {}

static void timer_timeout_handler(void* p_context)
{
    uint32_t in_pin_state = nrf_gpio_pin_read(GPIO_TICK_INPUT);

    if (in_pin_state != m_previous_inpin_state) {
        m_previous_inpin_state = in_pin_state;
        update_position();
    }

    if (m_previous_position != m_state.position) {
        m_idle_counter = -1;
    } else if (m_idle_counter == CTR_TIMER_TICKS_STOP_THRESHOLD) {
        NRF_LOG_PRINTF("No mov. Stopping at pos %d\r\n", m_state.position);
        app_timer_stop(m_app_ctrl_timer_id);
        sanitize_position();
        set_target_pos(NIL_POSITION);        

        m_previous_position = -10;
        m_inert_movement = MOVE_DIRECTION_NONE;
    }

    m_idle_counter++;
    m_previous_position = m_state.position;

    controller_call_cb();
}

/**@brief Function for the Timer initialization.
 *
 * @details Initializes the timer module. This creates and starts application timers.
 */
static void timers_init(void)
{
    // Initialize timer module.
    APP_TIMER_INIT(APP_CTRL_TIMER_PRESCALER, APP_CTRL_TIMER_OP_QUEUE_SIZE, false);

    app_timer_create(&m_app_ctrl_timer_id, APP_TIMER_MODE_REPEATED, timer_timeout_handler);
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

/*===========================================================================*/
/* Controller exported functions.                                            */
/*===========================================================================*/

void controller_init(int position)
{
    m_state.movement = MOVE_DIRECTION_NONE;
    m_state.position = position;
    m_state.target = NIL_POSITION;

    ret_code_t err_code;

    nrf_drv_gpiote_init();

    /* TICK Input pins config */
    nrf_drv_gpiote_in_config_t tick_in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    tick_in_config.pull = NRF_GPIO_PIN_PULLUP;

    err_code = nrf_drv_gpiote_in_init(GPIO_TICK_INPUT, &tick_in_config, in_pin_handler);
    APP_ERROR_CHECK(err_code);

    /* Output Pins config */
    nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

    err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_UP_PIN, &out_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_DOWN_PIN, &out_config);
    APP_ERROR_CHECK(err_code);

    err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_ENABLED_PIN, &out_config);
    APP_ERROR_CHECK(err_code);

    NRF_LOG_PRINTF("Ctrl init pos %d\r\n", position);

/* Tick generator init */
#if USE_TICK_GENERATOR
    tick_generator_init(GPIO_TICK_OUTPUT);
    NRF_LOG_PRINTF("Init tick generator");
#endif

    timers_init();
}

void controller_register_cb(controller_cb_t cb)
{
    m_cb = cb;
    controller_call_cb();
}

void controller_stop()
{
    set_target_pos(NIL_POSITION);
}

/**
 * @brief   Reset current controller position.
 * @note    Reset can be applied at current position or controller can try to reach the lowest possible position and then reset. 
 *          Function will take no effect during other movement or if resetting is already in progress.
 * 
 * @param[in] with_moving   bool indicating which behaviour should be used to reset
 */
void controller_extremum_position_set(uint8_t extremum)
{
    switch (extremum) {
    case CTRL_EXTREMUM_POS_BOTTOM:
        set_target_pos(-(int16_t)((uint16_t)~0 >> 1) - 1);
        break;
    case CTRL_EXTREMUM_POS_TOP:
        set_target_pos((int16_t)((uint16_t)~0 >> 1) - 1);
        break;
    default:
        break;
    }
}

void controller_target_position_set(int16_t target)
{
    set_target_pos(target);
}