#include "nrf.h"

void m45_init();
void m45pe_write(uint8_t key, uint8_t* val, uint8_t len);
void m45pe_read(uint8_t key, uint8_t* val, uint8_t len);