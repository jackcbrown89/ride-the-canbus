#include <math.h> 
#include <stdio.h>
#include <time.h>

#include "driver/gpio.h"
#include "driver/twai.h"

#include "main.h"
#include "operators.h" 


void process_message(twai_message_t *msg) {
    // no need to process remote request frames
    if (msg->rtr) {
        return;
    }

    int msg_id = msg->identifier;
    uint8_t data_length = msg->data_length_code;

    // printf("msg id %x\n", msg_id);
    for (int i = 0 ; i < NUM_MESSAGE_OPERATORS; i++) {
        if(msg_id == message_operators[i].message_id) {
            float mult_factor = message_operators[i].mult_factor;
            float add_factor = message_operators[i].add_factor;

            if (message_operators[i].bit_mode) {         
                // printf("Skipping bit mode operator...\n");                
                // uint8_t payload_le[msg->data_length_code];
                uint64_t value_le = 0;
                
                for (unsigned int j = 0 ; j < data_length ; j++) {
                    uint64_t shifted_value = ((uint64_t) msg->data[j] << (8 * j));
                    // printf("Payload[%d]: %llu\n", j, shifted_value);
                    // payload_le[j] = msg->data[data_length - j - 1];
                    value_le = value_le + shifted_value;
                    // printf("New value %llu\n", value_le);
                }
                // printf("%s: %llx\n", message_operators[i].name, value_le); 
                
                uint8_t num_bits = CHAR_BIT * sizeof(value_le);
                // printf("num bits %d", num_bits);
                value_le = value_le >> (message_operators[i].offset - num_bits);
                int mask = (1 << message_operators[i].length) - 1;
                int masked_value = value_le & mask;
                float result = (masked_value * mult_factor) + add_factor;
                printf(">%s:%f\n", message_operators[i].name, result);

            }
            else {
                int payload_start = message_operators[i].offset;
                int payload_end = payload_start + message_operators[i].length;
    
                if (message_operators[i].is_signed) {
                    int64_t value = 0;
                    uint8_t shift = 0;
                    for (int j = payload_start; j < payload_end; j++) {
                        int8_t datapoint = msg->data[j];
                        int64_t shifted_datapoint = (datapoint << shift);
                        value = value + shifted_datapoint;
                        shift = shift + 8;
                    }
                    value = value * mult_factor;
                    value = value + add_factor;
                    if (message_operators[i].has_mask) {
                        int64_t masked_value = value & message_operators[i].mask;
                        printf(">%s:%lld\n", message_operators[i].name, masked_value);
                    } else {
                        printf(">%s:%lld\n", message_operators[i].name, value);
                    }
                } else {
                    int64_t value = 0;
                    uint8_t shift = 0;
                    for (int j = payload_start; j < payload_end; j++) {
                        uint8_t datapoint = msg->data[j];
                        uint64_t shifted_datapoint = (datapoint << shift);
                        value = value + shifted_datapoint;
                        shift = shift + 8;
                    }
                    value = value * mult_factor;
                    value = value + add_factor;
                    // printf("%s:%d\n", message_operators[i].name, value);
                    printf(">%s:%lld\n", message_operators[i].name, value);
                }
            }
        }
    }
}

void app_main(void)
{
    twai_handle_t twai_bus_0;

    twai_general_config_t g_config_0 = SNIFFER_GENERAL_CONFIG;
    twai_timing_config_t t_config_0 = PT_CANBUS_TIMING_CONFIG;
    twai_filter_config_t f_config_0 = PT_CANBUS_FILTER_CONFIG;
    TickType_t RX_TIMEOUT = pdMS_TO_TICKS(5000);

    // Install driver for TWAI bus 0
    g_config_0.controller_id = 0;
    if (twai_driver_install_v2(&g_config_0, &t_config_0, &f_config_0, &twai_bus_0) == ESP_OK) {
        printf("TWAI [0] Driver installed\n");
    } else {
        printf("Failed to install TWAI [0] driver\n");
        return;
    }

    // Start TWAI driver
    if (twai_start_v2(twai_bus_0) == ESP_OK) {
        printf("TWAI [0] Driver started\n");
    } else {
        printf("Failed to start TWAI [0] driver\n");
        return;
    }

    // ESP_ERROR_CHECK(led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip));
    // uint32_t idx = 3;
    // uint32_t red = 0xFFFF;
    // uint32_t blue = 0xFFFF;
    // uint32_t green = 0;
    // ESP_ERROR_CHECK(led_strip_set_pixel(led_strip, idx, red, green, blue));

    int i = 1;
    while(true) {
        twai_message_t msg;
        if (twai_receive_v2(twai_bus_0, &msg, RX_TIMEOUT) == ESP_OK) {
            if ((i % 1000) == 0) {
                i = 1;
                twai_status_info_t twai_status;
                twai_get_status_info_v2(twai_bus_0, &twai_status);
                if (twai_status.rx_error_counter > 0) {
                    printf("RX_ERROR_COUNT=%lu\t", twai_status.rx_error_counter);
                }
                if (twai_status.rx_missed_count > 0) {
                    printf("RX_MISSED_COUNT=%lu\t", twai_status.rx_missed_count);
                }
                if (twai_status.rx_overrun_count > 0) {
                    printf("RX_OVERRUN_COUNT=%lu\t", twai_status.rx_overrun_count);
                }
                if (twai_status.msgs_to_rx > 0) {
                    printf("*******RX_QUEUE_SIZE=%lu\t", twai_status.rx_overrun_count);
                }
                printf("\n");
            }
            i++;
            // printf("Received msg\n");
            switch (msg.identifier) {
                case 0x0A5:
                case 0x0D9:
                case 0x0EF:
                case 0x1A1:
                case 0x302:
                    process_message(&msg);
                    break;
                default:
                    continue;
            }
            // process_message(&msg);
        }
        else {
            printf("Timeout or other error receiving message\n");
            continue;
        }
    }

}
