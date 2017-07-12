#ifndef CONTROLLER_H__
#define CONTROLLER_H__

#define MOVE_DIRECTION_UP			0xCB
#define MOVE_DIRECTION_DOWN		0x92
#define MOVE_DIRECTION_NONE		0x00

#define controller_stop() controller_move(MOVE_DIRECTION_NONE)

typedef void (*controller_pos_cb_t)(int);
typedef struct
{
		controller_pos_cb_t				pos_cb; 
} controller_cb_t;

void controller_init(void);
void controller_set_cb(controller_pos_cb_t cb);
void controller_move(uint8_t direction);
void controller_target_position_set(uint16_t position);

#endif
