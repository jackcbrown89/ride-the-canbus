#include <math.h> 
#include <stdio.h>

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
                
                int num_bits = (int) log2(value_le) + 1;
                value_le = value_le >> (message_operators[i].offset - num_bits);
                int mask = (1 << message_operators[i].length) - 1;
                int masked_value = value_le & mask;
                float result = (masked_value * mult_factor) + add_factor;
                printf("%s: %f\n", message_operators[i].name, result);

            }
            else {
                int payload_start = message_operators[i].offset;
                int payload_end = payload_start + message_operators[i].length;
    
                int value = 0;
                uint8_t shift = 0;
                for (int j = payload_start; j < payload_end; j++) {
                    uint8_t datapoint = msg->data[j];
                    unsigned int shifted_datapoint = (datapoint << shift);
                    value = value + shifted_datapoint;
                    shift = shift + 8;
                }
                value = value * mult_factor;
                value = value + add_factor;
                printf("%s: %d\n", message_operators[i].name, value);
            }
        }
    }
}

void app_main(void)
{
    uint64_t thign = 0xFF;
    printf("%llu\n", thign << 56);

    twai_handle_t twai_bus_0;

    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config_0 = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_10, GPIO_NUM_11, TWAI_MODE_LISTEN_ONLY);
    twai_timing_config_t t_config_0 = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config_0 = {.acceptance_code = 0, .acceptance_mask =0xFFFFFFFF, .single_filter = true};

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

    while(true) {
        twai_message_t msg;
        if (twai_receive_v2(twai_bus_0, &msg, pdMS_TO_TICKS(5000)) == ESP_OK) {
            // printf("Received msg\n");
            process_message(&msg);
        } else {
            // printf("Timeout or other error receiving message\n");
            continue;
        }
    }
   
}
