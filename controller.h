#ifndef CONTROLLER_H__
#define CONTROLLER_H__

#define MOVE_DIRECTION_UP			0xCB
#define MOVE_DIRECTION_DOWN		0x92
#define MOVE_DIRECTION_NONE		0x00

void controller_init(void);
void controller_move(uint8_t direction);
void controller_position_set(uint16_t position);

#endif
