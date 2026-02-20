#include "operators.h"

// Define message operators - these are examples, modify as needed for your CAN bus messages
const message_operator_t message_operators[NUM_MESSAGE_OPERATORS] = {
    {.name = "rpm", .message_id = 0x0A5, .mult_factor = 0.25, .add_factor = 0, .offset = 5, .length = 2, .bit_mode = false, .has_mask = 0, .mask = 0x0, .is_little_endian = true, .is_signed = false},
    {.name = "accelerator_pedal", .message_id = 0x0D9, .mult_factor = (1 / 40.0), .add_factor = 0, .offset = 16, .length = 12, .bit_mode = true, .has_mask = false, .mask = 0, .is_little_endian = true, .is_signed = true},
    {.name = "brake", .message_id = 0x0EF, .mult_factor = -2.0, .add_factor = 250.0, .offset = 3, .length = 1, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = true},
    {.name = "speed_mph", .message_id = 0x1A1, .mult_factor = 0.015625 / 1.60934, .add_factor = 0, .offset = 2, .length = 2, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = false},
    {.name = "gear", .message_id = 0x0F3, .mult_factor = 1, .add_factor = 0, .offset = 5, .length = 1, .bit_mode = false, .has_mask = true, .mask = 0xf, .is_little_endian = true, .is_signed = true},
    {.name = "Steering", .message_id = 0x302, .mult_factor = -1.0 / 22.75, .add_factor = 1440, .offset = 2, .length = 2, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = false},
    {.name = "Steering2", .message_id = 0x302, .mult_factor = 0.039555, .add_factor = -1296, .offset = 2, .length = 2, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = false},
};