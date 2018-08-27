#include "ctrl_service.h"
#include "app_error.h"
#include "aufzug_config.h"
#include "ble_srv_common.h"
#include "controller.h"
#include <stdint.h>
#include <string.h>

#define CTRL_COMMAND_FORCE_STOP 0xAA
#define CTRL_COMMAND_SET_TARGET_POS 0x60

#define CTRL_CHAR_LENGTH 3

static uint32_t ctrl_char_add(ble_ctrl_service_t* p_ctrl_service)
{
    uint32_t err_code;
    ble_uuid_t char_uuid;
    ble_uuid128_t base_uuid = BLE_UUID_CTRL_BASE_UUID;
    char_uuid.uuid = BLE_UUID_CTRL_CHARACTERISTC_UUID;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &char_uuid.type);
    APP_ERROR_CHECK(err_code);

    // Add read/write properties to our characteristic
    ble_gatts_char_md_t char_md;
    memset(&char_md, 0, sizeof(char_md));

    char_md.char_props.read = 0;
    char_md.char_props.write = 1;

    // Configuring Client Characteristic Configuration Descriptor metadata and add to char_md structure
    ble_gatts_attr_md_t cccd_md;
    memset(&cccd_md, 0, sizeof(cccd_md));

    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.read_perm);
    BLE_GAP_CONN_SEC_MODE_SET_OPEN(&cccd_md.write_perm);
    cccd_md.vloc = BLE_GATTS_VLOC_STACK;
    char_md.p_cccd_md = &cccd_md;
    char_md.char_props.notify = 0;

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
    attr_char_value.max_len = CTRL_CHAR_LENGTH;
    attr_char_value.init_len = CTRL_CHAR_LENGTH;
    uint8_t value[CTRL_CHAR_LENGTH] = { 0 };
    attr_char_value.p_value = value;

    // Add our new characteristic to the service
    err_code = sd_ble_gatts_characteristic_add(p_ctrl_service->service_handle,
        &char_md,
        &attr_char_value,
        &p_ctrl_service->char_handles);
    APP_ERROR_CHECK(err_code);

    return NRF_SUCCESS;
}

void control_service_init(ble_ctrl_service_t* p_ctrl_service)
{
    p_ctrl_service->conn_handle = BLE_CONN_HANDLE_INVALID;

    uint32_t err_code;
    ble_uuid_t service_uuid;
    ble_uuid128_t base_uuid = BLE_UUID_CTRL_BASE_UUID;

    service_uuid.uuid = BLE_UUID_CTRL_SERVICE;

    err_code = sd_ble_uuid_vs_add(&base_uuid, &service_uuid.type);
    APP_ERROR_CHECK(err_code);

    err_code = sd_ble_gatts_service_add(BLE_GATTS_SRVC_TYPE_PRIMARY,
        &service_uuid,
        &p_ctrl_service->service_handle);
    APP_ERROR_CHECK(err_code);

    ctrl_char_add(p_ctrl_service);
}

void ble_ctrl_service_on_cmd_set_target_pos(uint8_t* target)
{
    int16_t targetMm = 0;
    memcpy(&targetMm, target, sizeof(targetMm));

    int32_t targetUm = targetMm * 1000;
    int16_t targetPos = (targetUm - BASE_HEIGHT) / TICK_TO_HEIGHT_MULTI;
    controller_target_position_set(targetPos);
}

void ble_ctrl_service_on_write(ble_ctrl_service_t* p_ctrl_service, ble_evt_t* p_ble_evt)
{
    ble_gatts_evt_write_t* p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

    if (p_evt_write->handle == p_ctrl_service->char_handles.value_handle && p_evt_write->len > 0) {
        switch (p_evt_write->data[0]) {
        case CTRL_COMMAND_FORCE_STOP:
            controller_stop();
            break;
        case CTRL_COMMAND_SET_TARGET_POS:
            ble_ctrl_service_on_cmd_set_target_pos(&(p_evt_write->data[1]));
            break;
        default:
            break;
        }
    }
}

void ble_ctrl_service_on_ble_evt(ble_ctrl_service_t* p_ctrl_service, ble_evt_t* p_ble_evt)
{
    switch (p_ble_evt->header.evt_id) {
    case BLE_GAP_EVT_CONNECTED:
        p_ctrl_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
        break;
    case BLE_GAP_EVT_DISCONNECTED:
        p_ctrl_service->conn_handle = BLE_CONN_HANDLE_INVALID;
        break;
    case BLE_GATTS_EVT_WRITE:
        ble_ctrl_service_on_write(p_ctrl_service, p_ble_evt);
        break;
    default:
        // No implementation needed.
        break;
    }
}
