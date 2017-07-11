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

int position = 0;
int movement = MOVE_DIRECTION_NONE;

static bool isInit = false; 

void emergency_break() 
{
		nrf_gpio_pin_clear(GPIO_MOTOR_DOWN);
		nrf_gpio_pin_clear(GPIO_MOTOR_UP);
}

void in_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
		switch (movement) {
				case MOVE_DIRECTION_DOWN:
						position--;
						break;
				case MOVE_DIRECTION_UP:
						position++;
						break;
				case MOVE_DIRECTION_NONE:
						emergency_break();
						break;
				default: break;
		}
    //nrf_gpio_pin_toggle(GPIO_MOTOR_UP);
}

void switch_pin_handler(nrf_drv_gpiote_pin_t pin, nrf_gpiote_polarity_t action)
{
		SEGGER_RTT_printf(0, "Siemka"); // Print service UUID should match definition BLE_UUID_OUR_SERVICE
    nrf_gpio_pin_toggle(GPIO_SWITCH_CONTROL);
}

void controller_init(void)
{
		if (isInit) return;
	
		ret_code_t err_code;

    nrf_drv_gpiote_init();
	
		/* Input pins config */
		nrf_drv_gpiote_in_config_t in_config = GPIOTE_CONFIG_IN_SENSE_TOGGLE(true);
    in_config.pull = NRF_GPIO_PIN_PULLUP;
		
		err_code = nrf_drv_gpiote_in_init(GPIO_TICK_INPUT, &in_config, in_pin_handler);
    APP_ERROR_CHECK(err_code);
    nrf_drv_gpiote_in_event_enable(GPIO_TICK_INPUT, true);
	
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
		
}

void controller_position_set(uint16_t position)
{
}
