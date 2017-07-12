#include <stdbool.h>
#include "nrf.h"
#include "nrf_drv_gpiote.h"
#include "app_error.h"
#include "nrf_gpio.h"
#include "controller.h"
#include "boards.h"
#include "SEGGER_RTT.h"

#define GPIO_SWITCH_INPUT								25
#define GPIO_SWITCH_CONTROL							28
#define GPIO_MOTOR_UP										16
#define GPIO_MOTOR_DOWN									15
#define GPIO_TICK_INPUT 								29

#define DIRECTION_DEBUG(direction) direction == MOVE_DIRECTION_UP ? 1 : (direction == MOVE_DIRECTION_DOWN ? -1 : 0)

static controller_cb_t 		controller_cb;
static controller_state_t m_state;

static bool isInit = false; 

void emergency_break() 
{
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "Emergency break (!) with currentPosition %d and movement direction %d\n", m_state.position, DIRECTION_DEBUG(m_state.movement));
		#endif
		
		nrf_gpio_pin_clear(GPIO_MOTOR_DOWN);
		nrf_gpio_pin_clear(GPIO_MOTOR_UP);
}

void update_current_position(int position)
{
		m_state.position = position;
	
		if (controller_cb) controller_cb(&m_state);
}

void controller_register_cb(controller_cb_t cb)
{
		controller_cb = cb;
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "Tick pin handler with currentPosition %d, target %d, movement direction %d\n", m_state.position, m_state.target, DIRECTION_DEBUG(m_state.movement));
		#endif
	
		bool stop = m_state.target > -1;
	
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
						emergency_break();
						return;
				default: break;
		}
		
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "currentPosition changed to %d, stop? = %d\n", m_state.position, stop);
		#endif
		
		if (stop) {
				controller_stop();
		}
}

void switch_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
    nrf_gpio_pin_toggle(GPIO_SWITCH_CONTROL);
}

void controller_init(void)
{
		if (isInit) return;
	
		m_state.movement = MOVE_DIRECTION_NONE;
		m_state.position = 0;
		m_state.target = -1;
	
		ret_code_t err_code;

    nrf_drv_gpiote_init();
	
		/* TICK Input pins config */
		nrf_drv_gpiote_in_config_t tick_in_config = GPIOTE_CONFIG_IN_SENSE_HITOLO(true);
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
	
		isInit = true;
}

void controller_move(uint8_t direction) 
{
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "Controller move with direction %d (current %d)\n", DIRECTION_DEBUG(direction), DIRECTION_DEBUG(m_state.movement));
		#endif
	
		if (direction == m_state.movement) {
				return;
		}
	
		nrf_drv_gpiote_out_clear(GPIO_MOTOR_UP);
		nrf_drv_gpiote_out_clear(GPIO_MOTOR_DOWN);
		m_state.movement = direction;
	
		switch (direction) {
				case MOVE_DIRECTION_DOWN:
						nrf_drv_gpiote_out_set(GPIO_MOTOR_DOWN);
						break;
				case MOVE_DIRECTION_UP:
						nrf_drv_gpiote_out_set(GPIO_MOTOR_UP);
						break;
				case MOVE_DIRECTION_NONE:
				default: break;
		}
}

void controller_target_position_set(uint16_t target)
{
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "Set target currentPosition %d (%d current)\n", target, m_state.position);
		#endif
		
		if (target == m_state.position || target == m_state.target) {
				return;
		}
		
		m_state.target = target;
		
		if (target < m_state.position) {
				controller_move(MOVE_DIRECTION_DOWN);
		} else if (target > m_state.position) {
				controller_move(MOVE_DIRECTION_UP);
		}
}
