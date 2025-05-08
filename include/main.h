#ifndef MAIN_H
#define MAIN_H

#include "driver/twai.h"

// CAN bus configuration macros
#define SNIFFER_GENERAL_CONFIG             \
    {                                      \
        .mode = TWAI_MODE_LISTEN_ONLY,     \
        .tx_io = TWAI_IO_UNUSED,           \
        .rx_io = GPIO_NUM_21,              \
        .clkout_io = TWAI_IO_UNUSED,       \
        .bus_off_io = TWAI_IO_UNUSED,      \
        .tx_queue_len = 0,                 \
        .rx_queue_len = 100,               \
        .alerts_enabled = TWAI_ALERT_NONE, \
        .clkout_divider = 0,               \
        .intr_flags = ESP_INTR_FLAG_IRAM,  \
    }

// Define CAN bus timing configuration for common speeds
// 500 Kbps configuration
#define PT_CANBUS_TIMING_CONFIG   \
    {                             \
        .brp = 8,                 \
        .tseg_1 = 15,             \
        .tseg_2 = 4,              \
        .sjw = 3,                 \
        .triple_sampling = false, \
    }

// Define CAN bus filter (accept all messages)
#define PT_CANBUS_FILTER_CONFIG        \
    {                                  \
        .acceptance_code = 0,          \
        .acceptance_mask = 0xFFFFFFFF, \
        .single_filter = true,         \
    }

// Process a CAN message
void process_message(twai_message_t *msg);

#endif /* MAIN_H */