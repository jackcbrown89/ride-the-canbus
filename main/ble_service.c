#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_bt.h"
#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_bt_defs.h"
#include "esp_bt_main.h"
#include "esp_gatt_common_api.h"

#include "ble_service.h"
#include "main.h"

#define GATTS_SERVICE_UUID_CAN_DATA 0x00FF
#define GATTS_CHAR_UUID_CAN_DATA 0xFF01
#define GATTS_DESCR_UUID_CAN_DATA 0x3333
#define GATTS_NUM_HANDLE_CAN_DATA 4

#define DEVICE_NAME "ESP32_CAN_BLE"
#define MANUFACTURER_DATA_LEN 17

#define GATTS_DEMO_CHAR_VAL_LEN_MAX 500
#define PREPARE_BUF_MAX_SIZE 1024
#define CHAR_DECLARATION_SIZE (sizeof(uint8_t))

static uint8_t adv_config_done = 0;
#define adv_config_flag (1 << 0)
#define scan_rsp_config_flag (1 << 1)

#define TAG "CAN_BLE"

// UUIDs needed by GATT database - declared as extern in the header
const uint16_t primary_service_uuid = ESP_GATT_UUID_PRI_SERVICE;
const uint16_t character_declaration_uuid = ESP_GATT_UUID_CHAR_DECLARE;
const uint16_t character_client_config_uuid = ESP_GATT_UUID_CHAR_CLIENT_CONFIG;
const uint8_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// Make actual variables for the UUIDs
static uint16_t service_uuid_can_data = GATTS_SERVICE_UUID_CAN_DATA;
static uint16_t char_uuid_can_data = GATTS_CHAR_UUID_CAN_DATA;

static uint8_t service_uuid[16] = {
    /* LSB <--------------------------------------------------------------------------------> MSB */
    // first uuid, 16bit, [12],[13] is the value
    0xfb,
    0x34,
    0x9b,
    0x5f,
    0x80,
    0x00,
    0x00,
    0x80,
    0x00,
    0x10,
    0x00,
    0x00,
    0xFF,
    0x00,
    0x00,
    0x00,
};

/* The length of adv data must be less than 31 bytes */
static esp_ble_adv_data_t adv_data = {
    .set_scan_rsp = false,
    .include_name = true,
    .include_txpower = true,
    .min_interval = 0x0006, // slave connection min interval, Time = min_interval * 1.25 msec
    .max_interval = 0x0010, // slave connection max interval, Time = max_interval * 1.25 msec
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

// scan response data
static esp_ble_adv_data_t scan_rsp_data = {
    .set_scan_rsp = true,
    .include_name = true,
    .include_txpower = true,
    .appearance = 0x00,
    .manufacturer_len = 0,       // TEST_MANUFACTURER_DATA_LEN,
    .p_manufacturer_data = NULL, //&test_manufacturer[0],
    .service_data_len = 0,
    .p_service_data = NULL,
    .service_uuid_len = sizeof(service_uuid),
    .p_service_uuid = service_uuid,
    .flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT),
};

static esp_ble_adv_params_t adv_params = {
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};

struct gatts_profile_inst
{
    esp_gatts_cb_t gatts_cb;
    uint16_t gatts_if;
    uint16_t app_id;
    uint16_t conn_id;
    uint16_t service_handle;
    esp_gatt_srvc_id_t service_id;
    uint16_t char_handle;
    esp_bt_uuid_t char_uuid;
    esp_gatt_perm_t perm;
    esp_gatt_char_prop_t property;
    uint16_t descr_handle;
    esp_bt_uuid_t descr_uuid;
};

typedef struct
{
    uint8_t *prepare_buf;
    int prepare_len;
} prepare_type_env_t;

static prepare_type_env_t prepare_write_env;

/* One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT */
static struct gatts_profile_inst can_profile_tab[1];

/* CAN data notification state (enabled or disabled) */
static bool can_notification_enabled = false;

/* Queue to buffer CAN data */
static QueueHandle_t can_data_queue;

typedef struct
{
    char name[20];
    int64_t value;
} can_data_point_t;

// BLE connection status
static bool is_connected = false;

/* Full Database Description - Used to add attributes into the database */
static const esp_gatts_attr_db_t gatt_db[GATTS_NUM_HANDLE_CAN_DATA] = {
    // Service Declaration
    [0] = {
        .attr_control = {
            .auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16, .uuid_p = (uint8_t *)&primary_service_uuid, .perm = ESP_GATT_PERM_READ, .max_length = sizeof(uint16_t), .length = sizeof(uint16_t),
            .value = (uint8_t *)&service_uuid_can_data, // Fixed: Use variable instead of macro
        },
    },

    // CAN Data Characteristic Declaration
    [1] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&character_declaration_uuid,
            .perm = ESP_GATT_PERM_READ,
            .max_length = CHAR_DECLARATION_SIZE,
            .length = CHAR_DECLARATION_SIZE,
            .value = (uint8_t *)&char_prop_read_notify,
        },
    },

    // CAN Data Characteristic Value
    [2] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&char_uuid_can_data, // Fixed: Use variable instead of macro
            .perm = ESP_GATT_PERM_READ,
            .max_length = GATTS_DEMO_CHAR_VAL_LEN_MAX,
            .length = 0,
            .value = NULL,
        },
    },

    // CAN Data Characteristic - Client Characteristic Configuration Descriptor
    [3] = {
        .attr_control = {.auto_rsp = ESP_GATT_AUTO_RSP},
        .att_desc = {
            .uuid_length = ESP_UUID_LEN_16,
            .uuid_p = (uint8_t *)&character_client_config_uuid,
            .perm = ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
            .max_length = sizeof(uint16_t),
            .length = sizeof(uint16_t),
            .value = NULL,
        },
    },
};

static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GAP_BLE_ADV_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~adv_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_SCAN_RSP_DATA_SET_COMPLETE_EVT:
        adv_config_done &= (~scan_rsp_config_flag);
        if (adv_config_done == 0)
        {
            esp_ble_gap_start_advertising(&adv_params);
        }
        break;
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        if (param->adv_start_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Advertising start failed");
        }
        else
        {
            ESP_LOGI(TAG, "Advertising started");
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
        if (param->adv_stop_cmpl.status != ESP_BT_STATUS_SUCCESS)
        {
            ESP_LOGE(TAG, "Advertising stop failed");
        }
        else
        {
            ESP_LOGI(TAG, "Advertising stopped");
        }
        break;
    case ESP_GAP_BLE_UPDATE_CONN_PARAMS_EVT:
        ESP_LOGI(TAG, "Connection parameters updated");
        break;
    default:
        break;
    }
}

static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
        ESP_LOGI(TAG, "REGISTER_APP_EVT, status %d, app_id %d", param->reg.status, param->reg.app_id);
        can_profile_tab[0].service_id.is_primary = true;
        can_profile_tab[0].service_id.id.inst_id = 0x00;
        can_profile_tab[0].service_id.id.uuid.len = ESP_UUID_LEN_16;
        can_profile_tab[0].service_id.id.uuid.uuid.uuid16 = GATTS_SERVICE_UUID_CAN_DATA;

        esp_ble_gap_set_device_name(DEVICE_NAME);
        esp_ble_gap_config_adv_data(&adv_data);
        adv_config_done |= adv_config_flag;
        esp_ble_gap_config_adv_data(&scan_rsp_data);
        adv_config_done |= scan_rsp_config_flag;

        // Use gatt_db array here to create the attribute table
        esp_err_t create_attr_ret = esp_ble_gatts_create_attr_tab(gatt_db, gatts_if, GATTS_NUM_HANDLE_CAN_DATA, 0);

        if (create_attr_ret)
        {
            ESP_LOGE(TAG, "Create attr table failed, error code = %x", create_attr_ret);
        }

        // Still create service the classic way as backup
        esp_ble_gatts_create_service(gatts_if, &can_profile_tab[0].service_id, GATTS_NUM_HANDLE_CAN_DATA);
        break;
    case ESP_GATTS_CREATE_EVT:
        ESP_LOGI(TAG, "CREATE_SERVICE_EVT, status %d, service_handle %d", param->create.status, param->create.service_handle);
        can_profile_tab[0].service_handle = param->create.service_handle;
        esp_ble_gatts_start_service(can_profile_tab[0].service_handle);
        esp_ble_gatts_add_char(can_profile_tab[0].service_handle, &can_profile_tab[0].char_uuid,
                               ESP_GATT_PERM_READ,
                               ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY,
                               NULL, NULL);
        break;
    case ESP_GATTS_ADD_CHAR_EVT:
        ESP_LOGI(TAG, "ADD_CHAR_EVT, status %d, attr_handle %d, service_handle %d",
                 param->add_char.status, param->add_char.attr_handle, param->add_char.service_handle);
        can_profile_tab[0].char_handle = param->add_char.attr_handle;
        esp_ble_gatts_add_char_descr(can_profile_tab[0].service_handle,
                                     &can_profile_tab[0].descr_uuid,
                                     ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
                                     NULL, NULL);
        break;
    case ESP_GATTS_ADD_CHAR_DESCR_EVT:
        ESP_LOGI(TAG, "ADD_DESCR_EVT, status %d, attr_handle %d, service_handle %d",
                 param->add_char_descr.status, param->add_char_descr.attr_handle, param->add_char_descr.service_handle);
        can_profile_tab[0].descr_handle = param->add_char_descr.attr_handle;
        break;
    case ESP_GATTS_CONNECT_EVT:
        ESP_LOGI(TAG, "CONNECT_EVT, conn_id %d, remote %02x:%02x:%02x:%02x:%02x:%02x",
                 param->connect.conn_id,
                 param->connect.remote_bda[0], param->connect.remote_bda[1], param->connect.remote_bda[2],
                 param->connect.remote_bda[3], param->connect.remote_bda[4], param->connect.remote_bda[5]);
        can_profile_tab[0].conn_id = param->connect.conn_id;
        is_connected = true;
        break;
    case ESP_GATTS_DISCONNECT_EVT:
        ESP_LOGI(TAG, "DISCONNECT_EVT, reason %d", param->disconnect.reason);
        esp_ble_gap_start_advertising(&adv_params);
        is_connected = false;
        can_notification_enabled = false;
        break;
    case ESP_GATTS_READ_EVT:
        ESP_LOGI(TAG, "READ_EVT, handle %d", param->read.handle);
        break;
    case ESP_GATTS_WRITE_EVT:
        if (!param->write.is_prep)
        {
            // the data length of gattc write  must be less than GATTS_DEMO_CHAR_VAL_LEN_MAX.
            ESP_LOGI(TAG, "WRITE_EVT, handle %d, value len %d, value:", param->write.handle, param->write.len);
            if (can_profile_tab[0].descr_handle == param->write.handle && param->write.len == 2)
            {
                uint16_t descr_value = param->write.value[1] << 8 | param->write.value[0];
                if (descr_value == 0x0001)
                {
                    ESP_LOGI(TAG, "Notifications enabled");
                    can_notification_enabled = true;
                }
                else if (descr_value == 0x0000)
                {
                    ESP_LOGI(TAG, "Notifications disabled");
                    can_notification_enabled = false;
                }
            }
        }
        break;
    case ESP_GATTS_EXEC_WRITE_EVT:
        break;
    case ESP_GATTS_MTU_EVT:
        ESP_LOGI(TAG, "MTU %d", param->mtu.mtu);
        break;
    default:
        break;
    }
}

static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    /* If event is register event, store the gatts_if for each profile */
    if (event == ESP_GATTS_REG_EVT)
    {
        if (param->reg.status == ESP_GATT_OK)
        {
            can_profile_tab[0].gatts_if = gatts_if;
        }
        else
        {
            ESP_LOGI(TAG, "Register app failed, app_id %04x, status %d",
                     param->reg.app_id,
                     param->reg.status);
            return;
        }
    }

    /* Call each profile's callback */
    do
    {
        int idx;
        for (idx = 0; idx < 1; idx++)
        {
            if (gatts_if == ESP_GATT_IF_NONE || /* ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function */
                gatts_if == can_profile_tab[idx].gatts_if)
            {
                if (can_profile_tab[idx].gatts_cb)
                {
                    can_profile_tab[idx].gatts_cb(event, gatts_if, param);
                }
            }
        }
    } while (0);
}

// Task to send CAN data over BLE
void can_ble_task(void *pvParameter)
{
    can_data_point_t data_point;
    char notification_data[100];

    while (1)
    {
        if (xQueueReceive(can_data_queue, &data_point, portMAX_DELAY))
        {
            // Only send if connected and notifications are enabled
            if (is_connected && can_notification_enabled)
            {
                // Format data point as string
                int len = snprintf(notification_data, sizeof(notification_data),
                                   "%s:%lld", data_point.name, data_point.value);

                // Send notification
                esp_ble_gatts_send_indicate(can_profile_tab[0].gatts_if,
                                            can_profile_tab[0].conn_id,
                                            can_profile_tab[0].char_handle,
                                            len,
                                            (uint8_t *)notification_data,
                                            false);

                ESP_LOGI(TAG, "Sent notification: %s", notification_data);
            }
        }
    }
}

void log_datapoint(const char name[20], int64_t value)
{
    // Still print to console
    // printf(">%s:%lld\n", name, value);

    // Send to BLE if queue exists
    if (can_data_queue)
    {
        can_data_point_t data_point;
        strncpy(data_point.name, name, sizeof(data_point.name) - 1);
        data_point.name[sizeof(data_point.name) - 1] = '\0'; // Ensure null termination
        data_point.value = value;

        // Add to queue, don't block if full
        xQueueSend(can_data_queue, &data_point, 0);
    }
}

void ble_init(void)
{
    esp_err_t ret;

    // Initialize NVS - it is used to store BLE data
    ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));

    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
    ret = esp_bt_controller_init(&bt_cfg);
    if (ret)
    {
        ESP_LOGE(TAG, "%s initialize controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bt_controller_enable(ESP_BT_MODE_BLE);
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable controller failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_init();
    if (ret)
    {
        ESP_LOGE(TAG, "%s initialize bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_bluedroid_enable();
    if (ret)
    {
        ESP_LOGE(TAG, "%s enable bluedroid failed: %s", __func__, esp_err_to_name(ret));
        return;
    }

    ret = esp_ble_gatts_register_callback(gatts_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gap_register_callback(gap_event_handler);
    if (ret)
    {
        ESP_LOGE(TAG, "gap register error, error code = %x", ret);
        return;
    }

    ret = esp_ble_gatts_app_register(0);
    if (ret)
    {
        ESP_LOGE(TAG, "gatts app register error, error code = %x", ret);
        return;
    }

    esp_err_t local_mtu_ret = esp_ble_gatt_set_local_mtu(500);
    if (local_mtu_ret)
    {
        ESP_LOGE(TAG, "set local MTU failed, error code = %x", local_mtu_ret);
    }

    // Create queue for CAN data
    can_data_queue = xQueueCreate(20, sizeof(can_data_point_t));
    if (!can_data_queue)
    {
        ESP_LOGE(TAG, "Failed to create CAN data queue");
        return;
    }

    // Create task for sending BLE notifications
    xTaskCreate(can_ble_task, "can_ble_task", 4096, NULL, 5, NULL);

    // Initialize the profile
    can_profile_tab[0].gatts_cb = gatts_profile_event_handler;
    can_profile_tab[0].gatts_if = ESP_GATT_IF_NONE;

    // Initialize char UUID
    can_profile_tab[0].char_uuid.len = ESP_UUID_LEN_16;
    can_profile_tab[0].char_uuid.uuid.uuid16 = GATTS_CHAR_UUID_CAN_DATA;

    // Initialize descr UUID
    can_profile_tab[0].descr_uuid.len = ESP_UUID_LEN_16;
    can_profile_tab[0].descr_uuid.uuid.uuid16 = GATTS_DESCR_UUID_CAN_DATA;
}