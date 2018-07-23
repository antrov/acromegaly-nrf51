#include <stdint.h>
#include <string.h>
#include "status_service.h"
#include "ble_srv_common.h"
#include "app_error.h"
#include "nrf_gpio.h"

#define STATUS_CHAR_LENGTH 4

static uint32_t status_char_add(ble_ss_t *p_status_service)
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
	uint8_t value[STATUS_CHAR_LENGTH] = {0x00, 0x00, 0x00, 0x00};
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
void status_service_init(ble_ss_t *p_status_service)
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
	// STEP 3: Declare 16 bit service and 128 bit base UUIDs and add them to BLE stack table

	// STEP 4: Add our service

	// Print messages to Segger Real Time Terminal
	// UNCOMMENT THE FOUR LINES BELOW AFTER INITIALIZING THE SERVICE OR THE EXAMPLE WILL NOT COMPILE.
	//SEGGER_RTT_WriteString(0, "Exectuing our_service_init().\n"); // Print message to RTT to the application flow
	//SEGGER_RTT_printf(0, "Service UUID: 0x%#04x\n", service_uuid.uuid); // Print service UUID should match definition BLE_UUID_OUR_SERVICE
	//SEGGER_RTT_printf(0, "Service UUID type: 0x%#02x\n", service_uuid.type); // Print UUID type. Should match BLE_UUID_TYPE_VENDOR_BEGIN. Search for BLE_UUID_TYPES in ble_types.h for more info
	//SEGGER_RTT_printf(0, "Service handle: 0x%#04x\n", p_our_service->service_handle); // Print out the service handle. Should match service handle shown in MCP under Attribute values
}

void on_write(ble_ss_t *p_status_service, ble_evt_t *p_ble_evt)
{
	ble_gatts_evt_write_t *p_evt_write = &p_ble_evt->evt.gatts_evt.params.write;

	if (p_evt_write->handle == p_status_service->char_handles.value_handle && p_evt_write->len == 4)
	{
	}
}

void ble_status_service_on_ble_evt(ble_ss_t *p_status_service, ble_evt_t *p_ble_evt)
{
	switch (p_ble_evt->header.evt_id)
	{
	case BLE_GAP_EVT_CONNECTED:
		p_status_service->conn_handle = p_ble_evt->evt.gap_evt.conn_handle;
		break;
	case BLE_GAP_EVT_DISCONNECTED:
		p_status_service->conn_handle = BLE_CONN_HANDLE_INVALID;
		break;
	case BLE_GATTS_EVT_WRITE:
		on_write(p_status_service, p_ble_evt);
		break;
	default:
		// No implementation needed.
		break;
	}
}

void status_characteristic_update(ble_ss_t *p_status_service, uint8_t pos, uint8_t target, uint8_t mov, uint8_t sw)
{
	if (p_status_service->conn_handle != BLE_CONN_HANDLE_INVALID)
	{
		ble_gatts_hvx_params_t hvx_params;
		memset(&hvx_params, 0, sizeof(hvx_params));
		uint16_t len = STATUS_CHAR_LENGTH;
		uint8_t value[STATUS_CHAR_LENGTH];
		memset(&value, 0, sizeof(uint8_t) * len);

		value[0] = pos;
		value[1] = target;
		value[2] = mov;
		value[3] = sw;

		hvx_params.handle = p_status_service->char_handles.value_handle;
		hvx_params.type = BLE_GATT_HVX_NOTIFICATION;
		hvx_params.offset = 0;
		hvx_params.p_len = &len;
		hvx_params.p_data = value;

		sd_ble_gatts_hvx(p_status_service->conn_handle, &hvx_params);
	}
}
