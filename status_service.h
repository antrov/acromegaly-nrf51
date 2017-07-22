#ifndef STATUS_SERVICE_H__
#define STATUS_SERVICE_H__

#include <stdint.h>
#include "ble.h"
#include "ble_srv_common.h"

#define BLE_UUID_STATUS_BASE_UUID              {0x23, 0xD1, 0x13, 0xEF, 0x5F, 0x78, 0x23, 0x15, 0xDE, 0xEF, 0x12, 0x12, 0x00, 0x00, 0x00, 0x00} // 128-bit base UUID
#define BLE_UUID_STATUS_SERVICE                0xABCD // Just a random, but recognizable value
#define BLE_UUID_STATUS_CHARACTERISTC_UUID			0xBEEF

/**
 * @brief This structure contains various status information for our service. 
 * It only holds one entry now, but will be populated with more items as we go.
 * The name is based on the naming convention used in Nordic's SDKs. 
 * 'ble� indicates that it is a Bluetooth Low Energy relevant structure and 
 * �os� is short for Our Service). 
 */
typedef struct
{
		uint16_t                    conn_handle; 
    uint16_t    								service_handle; 
		ble_gatts_char_handles_t    char_handles;
} ble_ss_t;

void ble_status_service_on_ble_evt(ble_ss_t * p_status_service, ble_evt_t * p_ble_evt);

/**@brief Function for initializing our new service.
 *
 * @param[in]   p_our_service       Pointer to Our Service structure.
 */
void status_service_init(ble_ss_t * p_status_service);

void status_characteristic_update(ble_ss_t *p_status_service, uint8_t pos, uint8_t target, uint8_t mov, uint8_t sw);

#endif  /* _ OUR_SERVICE_H__ */