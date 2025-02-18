#include <stdio.h>

#include "driver/gpio.h"
#include "driver/twai.h"

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
    if (!msg->rtr) {
        printf("%d\n", msg_id);
        for (int i = 0; i < msg->data_length_code; i++) {
            printf("[%x]", msg->data[i]);
        }
        printf("\n");
    }
}

void app_main(void)
{
    twai_handle_t twai_bus_0;
    // twai_handle_t twai_bus_1;

    // Initialize configuration structures using macro initializers
    twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_0, GPIO_NUM_1, TWAI_MODE_NORMAL);
    twai_timing_config_t t_config = TWAI_TIMING_CONFIG_500KBITS();
    twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

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
            // printf("Received message!\n");
            process_message(&msg);
        } else {
            printf("Timeout or other error receiving message\n");
            continue;
        }
    }
   
}
