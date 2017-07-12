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

int movement = MOVE_DIRECTION_NONE;
int currentPosition = 0;
int targetPositon = -1;

static controller_cb_t controller_cb;

static bool isInit = false; 

void emergency_break() 
{
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "Emergency break (!) with currentPosition %d and movement direction %d\n", currentPosition, DIRECTION_DEBUG(movement));
		#endif
		
		nrf_gpio_pin_clear(GPIO_MOTOR_DOWN);
		nrf_gpio_pin_clear(GPIO_MOTOR_UP);
}

void update_current_position(int position)
{
		currentPosition = position;
	
		if (controller_cb.pos_cb) 
		{
				controller_cb.pos_cb(position);
		}
}

void controller_set_cb(controller_pos_cb_t cb)
{
		controller_cb.pos_cb = cb;
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "Tick pin handler with currentPosition %d, target %d, movement direction %d\n", currentPosition, targetPositon, DIRECTION_DEBUG(movement));
		#endif
	
		bool stop = targetPositon > -1;
	
		switch (movement) {
				case MOVE_DIRECTION_DOWN:
						update_current_position(currentPosition - 1);
						stop &= currentPosition <= targetPositon;
						break;
				case MOVE_DIRECTION_UP:
						update_current_position(currentPosition + 1);
						stop &= currentPosition >= targetPositon;
						break;
				case MOVE_DIRECTION_NONE:
						emergency_break();
						return;
				default: break;
		}
		
		#if DEBUG == 1
		SEGGER_RTT_printf(0, "currentPosition changed to %d, stop? = %d\n", currentPosition, stop);
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
		SEGGER_RTT_printf(0, "Controller move with direction %d (current %d)\n", DIRECTION_DEBUG(direction), DIRECTION_DEBUG(movement));
		#endif
	
		if (direction == movement) {
				return;
		}
	
		nrf_drv_gpiote_out_clear(GPIO_MOTOR_UP);
		nrf_drv_gpiote_out_clear(GPIO_MOTOR_DOWN);
		movement = direction;
	
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
		SEGGER_RTT_printf(0, "Set target currentPosition %d (%d current)\n", target, currentPosition);
		#endif
		
		if (target == currentPosition || target == targetPositon) {
				return;
		}
		
		targetPositon = target;
		
		if (target < currentPosition) {
				controller_move(MOVE_DIRECTION_DOWN);
		} else if (target > currentPosition) {
				controller_move(MOVE_DIRECTION_UP);
		}
}
