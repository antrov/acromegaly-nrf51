#include "nrf_gpio.h"
#include "controller.h"

#define GPIO_SWITCH_INPUT								25
#define GPIO_SWITCH_CONTROL							28
#define GPIO_MOTOR_UP										16
#define GPIO_MOTOR_DOWN									15

static bool isInit = false; 

void controller_init(void)
{
		if (isInit) return;
	
		nrf_gpio_cfg_input(GPIO_SWITCH_INPUT, NRF_GPIO_PIN_PULLUP);
		nrf_gpio_cfg_output(GPIO_MOTOR_UP);
		nrf_gpio_cfg_output(GPIO_MOTOR_DOWN);
		nrf_gpio_cfg_output(GPIO_SWITCH_CONTROL);
	
		nrf_gpio_pin_write(GPIO_MOTOR_UP, 1);
		nrf_gpio_pin_write(GPIO_MOTOR_DOWN, 1);
		nrf_gpio_pin_write(GPIO_SWITCH_CONTROL, 1);
	
		isInit = true;
}
