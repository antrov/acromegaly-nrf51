#include "app_error.h"
#include "app_pwm.h"
#include "tick_generator.h"
#include "acromegaly_config.h"

#if USE_TICK_GENERATOR
APP_PWM_INSTANCE(PWM1,1); 

static volatile bool state_flag;
static volatile bool ready_flag;   

void pwm_ready_callback(uint32_t pwm_id)    // PWM callback function
{
    ready_flag = true;
}

void tick_generator_init(uint8_t pin)
{
		ret_code_t err_code;
	
		app_pwm_config_t pwm1_cfg = APP_PWM_DEFAULT_CONFIG_1CH(83000L, pin);
		err_code = app_pwm_init(&PWM1,&pwm1_cfg,pwm_ready_callback);
    APP_ERROR_CHECK(err_code);
		
    app_pwm_enable(&PWM1);	
		tick_generator_stop();
}

void tick_generator_start(void)
{
		while (app_pwm_channel_duty_set(&PWM1, 0, 50) == NRF_ERROR_BUSY);
		state_flag = true;
}

void tick_generator_stop(void)
{
		while (app_pwm_channel_duty_set(&PWM1, 0, 100) == NRF_ERROR_BUSY);
		state_flag = false;
}
#endif
