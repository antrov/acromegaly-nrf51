#ifndef CTRL_SERVICE_H__
#define CTRL_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

#define BLE_UUID_CTRL_BASE_UUID              	{{0x23, 0x05, 0x19, 0x90, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00}} // 128-bit base UUID
#define BLE_UUID_CTRL_SERVICE                	0xB00B
#define BLE_UUID_CTRL_CHARACTERISTC_UUID			0xBEEF

typedef struct
{
		uint16_t                    conn_handle; 
    uint16_t    								service_handle; 
		ble_gatts_char_handles_t    char_handles;
} ble_ctrl_service_t;

void control_service_init(ble_ctrl_service_t * p_ctrl_service);
void ble_ctrl_service_on_ble_evt(ble_ctrl_service_t * p_ctrl_service, ble_evt_t * p_ble_evt);

#endif /* ctrl_service */
