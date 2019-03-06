#include "status_service.h"
#include "app_error.h"
#include "acromegaly_config.h"
#include "ble_srv_common.h"
#include "nrf_gpio.h"
#include "nrf_log.h"
#include <string.h>

#define STATUS_CHAR_LENGTH 9

static uint32_t status_char_add(ble_status_service_t* p_status_service)
{
    uint32_t err_code;
    ble_uuid_t char_uuid;
    ble_uuid128_t base_uuid = BLE_UUID_STATUS_BASE_UUID;
    char_uuid.uuid = BLE_UUID_STATUS_CHARACTERISTC_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    APP_ERROR_CHECK(err_code);

    // Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read = 1;
    char_md.char_props.write = 0;

    // Configuring Client Characteristic Configuration Descriptor metadata and add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md = &cccd_md;
    char_md.char_props.notify = 1;

    // Configure the attribute metadata
    ble_gatts_attr_md_t attr_md;
    memset(&attr_md, 0, sizeof(attr_md));

    attr_md.vloc = BLE_GATTS_VLOC_STACK;
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&attr_md.write_perm);

    // Configure the characteristic value attribute
    ble_gatts_attr_t attr_char_value;
    memset(&attr_char_value, 0, sizeof(attr_char_value));

    attr_char_value.p_uuid = &char_uuid;
    attr_char_value.p_attr_md = &attr_md;
    attr_char_value.max_len = STATUS_CHAR_LENGTH;
    attr_char_value.init_len = STATUS_CHAR_LENGTH;
    uint8_t value[STATUS_CHAR_LENGTH] = { 0 };
    attr_char_value.p_value = value;

    // motor control (up, down), motor position (cm), global switch value (on/of), power consumption

    // Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_status_service->service_handle,
        &char_md,
        &attr_char_value,
        &p_status_service->char_handles);
    APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}

/**@brief Function for initiating our new service.
 *
 * @param[in]   p_our_service        Our Service structure.
 *
 */
void status_service_init(ble_status_service_t* p_status_service)
{
    p_status_service->conn_handle = BLE_CONN_HANDLE_INVALID;

    uint32_t err_code;
    ble_uuid_t service_uuid;
    ble_uuid128_t base_uuid = BLE_UUID_STATUS_BASE_UUID;

    service_uuid.uuid = BLE_UUID_STATUS_SERVICE;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
        &service_uuid,
        &p_status_service->service_handle);
    APP_ERROR_CHECK(err_code);

    status_char_add(p_status_service);
}

void ble_status_service_on_ble_evt(ble_status_service_t* p_status_service, ble_evt_t* p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
        p_status_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        break;
    case BLE_GAP_EVT_DISCONNECTED:
        p_status_service->conn_handle = BLE_CONN_HANDLE_INVALID;
        break;
    case BLE_GATTS_EVT_WRITE:
        break;
    default:
        // No implementation needed.
        break;
    }
}

void status_characteristic_update(ble_status_service_t* p_status_service, int16_t pos, int16_t target, uint8_t target_type, uint8_t mov)
{
    if (p_status_service->conn_handle != BLE_CONN_HANDLE_INVALID) {
        ble_gatts_hvx_params_t hvx_params;
        memset(&hvx_params, 0, sizeof(hvx_params));
        uint16_t len = STATUS_CHAR_LENGTH;
        uint8_t value[STATUS_CHAR_LENGTH] = { 0 };

        int32_t umPosition = ((int32_t)pos * TICK_TO_HEIGHT_MULTI) + BASE_HEIGHT;
        int32_t umTarget = target > 0 ? ((int32_t)target * TICK_TO_HEIGHT_MULTI) + BASE_HEIGHT : 0;

        int16_t mmPosition = umPosition / 1000;
        int16_t mmTarget = umTarget / 1000;

        // if (mov == 0xA1) {
            // NRF_LOG_PRINTF("Stat: %d (@ %d), type: %x\r\n", umPosition, pos, target_type);
        // }

        memcpy(value, (uint8_t*)&mmPosition, sizeof(int16_t));
        memcpy(value + sizeof(int16_t), (uint8_t*)&mmTarget, sizeof(int16_t));

        value[4] = target_type;
        value[5] = mov;

        hvx_params.handle = p_status_service->char_handles.value_handle;
        hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
        hvx_params.offset = 0;
        hvx_params.p_len = &len;
        hvx_params.p_data = value;

        sd_ble_gatts_hvx(p_status_service->conn_handle, &hvx_params);
    }
}
