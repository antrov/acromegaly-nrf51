#include <stdbool.h>
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "app_pwm.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include "controller.h"
#include "boards.h"
#include "aufzug_config.h"
#include "tick_generator.h"

#define GPIO_SWITCH_INPUT 25
#define GPIO_SWITCH_CONTROL 28
#define GPIO_MOTOR_UP 16
#define GPIO_MOTOR_DOWN 15
#define GPIO_TICK_INPUT 29
#define GPIO_TICK_OUTPUT 01 /* Tick PWM generator, fo testing purpoese */

#define MIN_POSITION 0	/* Defined in cm, different from NIL_POSITION */
#define MAX_POSITION 1000 /* Defined in cm */
#define NIL_POSITION -1   /* Marks target as unset */

#define DIRECTION_DEBUG(direction) direction == MOVE_DIRECTION_UP ? 1 : (direction == MOVE_DIRECTION_DOWN ? -1 : 0)

static controller_cb_t m_cb;	   /* Current state of controller */
static controller_state_t m_state; /* COntroller state callback method. Optional */

static bool isInit = false; /* For single init of controller */

void update_current_position(int position);
void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);
void switch_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action);

void update_current_position(int position)
{
	m_state.position = position;

	if (m_cb)
		m_cb(&m_state);
}

void controller_init(void)
{
	if (isInit)
		return;

	m_state.movement = MOVE_DIRECTION_NONE;
	m_state.position = 500; //MIN_POSITION;
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

	/* Switch Input pins config */
	nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
	in_config.pull = NRF_GPIO_PIN_PULLUP;
	err_code = nrf_drv_gpiote_in_init(GPIO_SWITCH_INPUT, &in_config, switch_pin_handler);
	APP_ERROR_CHECK(err_code);
	nrf_drv_gpiote_in_event_enable(GPIO_SWITCH_INPUT, true);

	/* Output Pins config */
	nrf_drv_gpiote_out_config_t out_config = GPIOTE_CONFIG_OUT_SIMPLE(false);

	err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_UP, &out_config);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_gpiote_out_init(GPIO_MOTOR_DOWN, &out_config);
	APP_ERROR_CHECK(err_code);

	err_code = nrf_drv_gpiote_out_init(GPIO_SWITCH_CONTROL, &out_config);
	APP_ERROR_CHECK(err_code);

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
	if (m_state.movement != MOVE_DIRECTION_NONE)
	{
		tick_generator_start();
		NRF_LOG_PRINTF("Startted tick generator\n");
	}
	else
	{
		tick_generator_stop();
		NRF_LOG_PRINTF("Stopped tick generator\n");
	}
}
#endif

void controller_register_cb(controller_cb_t cb)
{
	m_cb = cb;
}

void controller_move(uint8_t direction)
{
	nrf_drv_gpiote_out_clear(GPIO_MOTOR_UP);
	nrf_drv_gpiote_out_clear(GPIO_MOTOR_DOWN);
	m_state.movement = MOVE_DIRECTION_NONE;

	uint8_t motor = 0x00;

	switch (direction)
	{
	case MOVE_DIRECTION_DOWN:
		motor = m_state.position > MIN_POSITION ? GPIO_MOTOR_DOWN : 0;
		break;
	case MOVE_DIRECTION_UP:
		motor = m_state.position < MAX_POSITION ? GPIO_MOTOR_UP : 0;
		break;
	case MOVE_DIRECTION_NONE:
	default:
		break;
	}

	if (motor == 0x00)
	{
		NRF_LOG_PRINTF("Controller move not allowed. Stopping\n");
	}
	else
	{
		NRF_LOG_PRINTF("Controller move with direction %d (current %d) from positon %d\n", DIRECTION_DEBUG(direction), DIRECTION_DEBUG(m_state.movement), m_state.position);

		m_state.movement = direction;
		nrf_drv_gpiote_out_set(motor);
	}

#if USE_TICK_GENERATOR
	update_tick_generator();
#endif

	if (m_cb)
		m_cb(&m_state);
}

void controller_target_position_set(int16_t target)
{
	if (target == NIL_POSITION)
	{
		m_state.target = NIL_POSITION;
		controller_stop();
		return;
	}

	target = target < MIN_POSITION ? MIN_POSITION : (target > MAX_POSITION ? MAX_POSITION : target);

	if (target == m_state.position || target == m_state.target)
	{
		return;
	}

	NRF_LOG_PRINTF("Set target currentPosition %d (%d current)\n", target, m_state.position);

	m_state.target = target;

	if (target < m_state.position)
	{
		controller_move(MOVE_DIRECTION_DOWN);
	}
	else if (target > m_state.position)
	{
		controller_move(MOVE_DIRECTION_UP);
	}
	else
		return;

	if (m_cb)
		m_cb(&m_state);
}

void controller_switch(uint8_t switch_state)
{

	NRF_LOG_PRINTF("Controller switch to %d from %d (read %ld)\n", switch_state, m_state.global_switch, nrf_gpio_pin_read(GPIO_SWITCH_CONTROL));


	if (m_state.global_switch == nrf_gpio_pin_read(GPIO_SWITCH_CONTROL) && m_state.global_switch == switch_state)
	{
		return;
	}

	if (switch_state == SWITCH_OFF)
	{
		nrf_drv_gpiote_out_clear(GPIO_SWITCH_CONTROL);
	}
	else if (switch_state == SWITCH_ON)
	{
		nrf_drv_gpiote_out_set(GPIO_SWITCH_CONTROL);
	}
	else
		return;

	m_state.global_switch = switch_state;
	if (m_cb)
		m_cb(&m_state);
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{

	NRF_LOG_PRINTF("Tick pin handler with currentPosition %d, target %d, movement direction %d\n", m_state.position, m_state.target, DIRECTION_DEBUG(m_state.movement));


	bool target = m_state.target != NIL_POSITION;
	bool stop = false;

	switch (m_state.movement)
	{
	case MOVE_DIRECTION_DOWN:
		update_current_position(m_state.position - 1);
		target &= m_state.position <= m_state.target;
		stop = m_state.position <= MIN_POSITION;
		break;
	case MOVE_DIRECTION_UP:
		update_current_position(m_state.position + 1);
		target &= m_state.position >= m_state.target;
		stop = m_state.position >= MAX_POSITION;
		break;
	case MOVE_DIRECTION_NONE:
		stop = true;
		return;
	default:
		break;
	}

	NRF_LOG_PRINTF("currentPosition changed to %d, stop? = %d\n", m_state.position, stop);

	if (stop || target)
	{
		controller_stop();
	}

	if (target)
	{
		m_state.target = NIL_POSITION;
		if (m_cb)
			m_cb(&m_state);
	}
}

void switch_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
	controller_switch(m_state.global_switch == SWITCH_OFF ? SWITCH_ON : SWITCH_OFF);
}
