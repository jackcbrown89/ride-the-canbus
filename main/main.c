#include <math.h>
#include <stdio.h>
#include <time.h>

#include "driver/gpio.h"
#include "driver/twai.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include "main.h"
#include "operators.h"
#include "ble_service.h"

// Queue handle for CAN messages
static QueueHandle_t can_msg_queue;
#define CAN_MSG_QUEUE_SIZE 10

// Task handle for CAN message processing
static TaskHandle_t can_process_task_handle;

void process_message(twai_message_t *msg)
{
    // no need to process remote request frames
    if (msg->rtr)
    {
        return;
    }

    int msg_id = msg->identifier;
    uint8_t data_length = msg->data_length_code;

    for (int i = 0; i < NUM_MESSAGE_OPERATORS; i++)
    {
        if (msg_id == message_operators[i].message_id)
        {
            float mult_factor = message_operators[i].mult_factor;
            float add_factor = message_operators[i].add_factor;

            if (message_operators[i].bit_mode)
            {
                uint64_t value_le = 0;

                for (unsigned int j = 0; j < data_length; j++)
                {
                    uint64_t shifted_value = ((uint64_t)msg->data[j] << (8 * j));
                    value_le = value_le + shifted_value;
                }

                uint8_t num_bits = CHAR_BIT * sizeof(value_le);
                value_le = value_le >> (message_operators[i].offset - num_bits);
                int mask = (1 << message_operators[i].length) - 1;
                int masked_value = value_le & mask;
                float result = (masked_value * mult_factor) + add_factor;

                log_datapoint(message_operators[i].name, result);
            }
            else
            {
                int payload_start = message_operators[i].offset;
                int payload_end = payload_start + message_operators[i].length;

                if (message_operators[i].is_signed)
                {
                    int64_t value = 0;
                    uint8_t shift = 0;
                    for (int j = payload_start; j < payload_end; j++)
                    {
                        int8_t datapoint = msg->data[j];
                        int64_t shifted_datapoint = (datapoint << shift);
                        value = value + shifted_datapoint;
                        shift = shift + 8;
                    }
                    value = value * mult_factor;
                    value = value + add_factor;
                    if (message_operators[i].has_mask)
                    {
                        int64_t masked_value = value & message_operators[i].mask;

                        log_datapoint(message_operators[i].name, masked_value);
                    }
                    else
                    {
                        log_datapoint(message_operators[i].name, value);
                    }
                }
                else
                {
                    int64_t value = 0;
                    uint8_t shift = 0;
                    for (int j = payload_start; j < payload_end; j++)
                    {
                        uint8_t datapoint = msg->data[j];
                        uint64_t shifted_datapoint = (datapoint << shift);
                        value = value + shifted_datapoint;
                        shift = shift + 8;
                    }
                    value = value * mult_factor;
                    value = value + add_factor;

                    log_datapoint(message_operators[i].name, value);
                }
            }
        }
    }
}

// Task to process CAN messages
void can_process_task(void *pvParameter)
{
    twai_message_t msg;

    while (1)
    {
        // Wait for message from queue
        if (xQueueReceive(can_msg_queue, &msg, portMAX_DELAY))
        {
            process_message(&msg);
        }
    }
}

void app_main(void)
{
    ESP_LOGI("CAN_APP", "Starting CAN BUS application with BLE support");

    // Initialize BLE first
    ble_init();

    ESP_LOGI("CAN_APP", "BLE initialized, starting CAN bus");

    // Create queue for CAN messages
    can_msg_queue = xQueueCreate(CAN_MSG_QUEUE_SIZE, sizeof(twai_message_t));
    if (can_msg_queue == NULL)
    {
        ESP_LOGE("CAN_APP", "Failed to create CAN message queue");
        return;
    }

    // Create task for processing CAN messages
    BaseType_t task_created = xTaskCreate(
        can_process_task,
        "can_process_task",
        4096,
        NULL,
        5,
        &can_process_task_handle);

    if (task_created != pdPASS)
    {
        ESP_LOGE("CAN_APP", "Failed to create CAN processing task");
        return;
    }

    twai_handle_t twai_bus_0;

    const gpio_num_t TX_GPIO = GPIO_NUM_0;
    const gpio_num_t RX_GPIO = GPIO_NUM_1;
    twai_general_config_t g_config_0 = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config_0 = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config_0 = {.acceptance_code = 0, .acceptance_mask = 0xFFFFFFFF, .single_filter = true};
    TickType_t RX_TIMEOUT = pdMS_TO_TICKS(5000);

    // Install driver for TWAI bus 0
    g_config_0.controller_id = 0;
    if (twai_driver_install_v2(&g_config_0, &t_config_0, &f_config_0, &twai_bus_0) == ESP_OK)
    {
        ESP_LOGI("CAN_APP", "TWAI [0] Driver installed");
    }
    else
    {
        ESP_LOGE("CAN_APP", "Failed to install TWAI [0] driver");
        return;
    }

    // Start TWAI driver
    if (twai_start_v2(twai_bus_0) == ESP_OK)
    {
        ESP_LOGI("CAN_APP", "TWAI [0] Driver started");
    }
    else
    {
        ESP_LOGE("CAN_APP", "Failed to start TWAI [0] driver");
        return;
    }

    while (true)
    {
        twai_message_t msg;
        if (twai_receive_v2(twai_bus_0, &msg, RX_TIMEOUT) == ESP_OK)
        {
            switch (msg.identifier)
            {
            case 0x0A5:
            case 0x0D9:
            case 0x0EF:
            case 0x0F3:
            case 0x1A1:
            case 0x302:
                // Send message to queue instead of processing directly
                // if (xQueueSend(can_msg_queue, &msg, 0) != pdPASS)
                // {
                //     ESP_LOGW("CAN_APP", "Failed to queue CAN message - queue full");
                // }
                break;
            default:
                continue;
            }
        }
        else
        {
            ESP_LOGW("CAN_APP", "Timeout or other error receiving message");
            continue;
        }
    }
}