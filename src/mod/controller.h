#ifndef CONTROLLER_H__
#define CONTROLLER_H__

#include <stdint.h>
#include <string.h>

#define MOVE_DIRECTION_UP			0xCB
#define MOVE_DIRECTION_DOWN		0x92
#define MOVE_DIRECTION_NONE		0xA1
#define SWITCH_OFF						0x00
#define SWITCH_ON							0x01

#define controller_stop() controller_move(MOVE_DIRECTION_NONE)

typedef struct 
{
		int16_t 		position;
		int16_t 		target;
		uint8_t 		movement;
		uint8_t			global_switch;
} controller_state_t;

typedef void (*controller_cb_t)(controller_state_t* block);

void controller_init(int position);
void controller_register_cb(controller_cb_t cb);
void controller_move(uint8_t direction);
void controller_target_position_set(int16_t position);
void controller_switch(uint8_t switch_state);

#endif
