#include <stdio.h>

#include "driver/gpio.h"
#include "driver/twai.h"

#include "main.h"
#include "operators.h" 

void process_rpm_message(twai_message_t *msg) {
    int msg_id = msg->identifier;
    if (!msg->rtr) {
        printf("%d\n", msg_id);
        for (int i = 0; i < msg->data_length_code; i++) {
            printf("[%x]", msg->data[i]);
        }
        printf("\n");
    }
}

void process_message(twai_message_t *msg) {
    int msg_id = msg->identifier;

    for (int i = 0 ; i < NUM_MESSAGE_OPERATORS; i++) {
        if(msg_id == message_operators[i].message_id) {
            float mult_factor = message_operators[i].mult_factor;
            float add_factor = message_operators[i].add_factor;
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
    // have a list of message identifiers that you need to check


    // if (!msg->rtr) {
    //     printf("%d\n", msg_id);
    //     for (int i = 0; i < msg->data_length_code; i++) {
    //         printf("[%x]", msg->data[i]);
    //     }
    //     printf("\n");
    // }
}

void app_main(void)
{
    twai_handle_t twai_bus_0;
    // twai_handle_t twai_bus_1;

    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_0, GPIO_NUM_1, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = {.acceptance_code = 0, .acceptance_mask = 0xFFFFFFFF, .single_filter = true};

    // Install driver for TWAI bus 0
    g_config.controller_id = 0;
    if (twai_driver_install_v2(&g_config, &t_config, &f_config, &twai_bus_0) == ESP_OK) {
        printf("TWAI Driver installed\n");
    } else {
        printf("Failed to install TWAI driver\n");
        return;
    }

    // Start TWAI driver
    if (twai_start_v2(twai_bus_0) == ESP_OK) {
        printf("TWAI Driver started\n");
    } else {
        printf("Failed to start TWAI driver\n");
        return;
    }

    while(true) {
        twai_message_t msg;
        if (twai_receive_v2(twai_bus_0, &msg, pdMS_TO_TICKS(5000)) == ESP_OK) {
            process_message(&msg);
        } else {
            printf("Timeout or other error receiving message\n");
            continue;
        }
    }
   
}
