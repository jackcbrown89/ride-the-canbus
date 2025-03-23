typedef struct
{
    char name[20];
    int message_id;
    float mult_factor;
    float add_factor;
    int offset;
    int length;
    bool bit_mode;
    bool has_mask;
    int mask;
    bool is_little_endian;
    bool is_signed;
} Message_operator;

/**
 * "Bit mode" example
 * [0x2F, 0xF5, 0xA3] = [00101111, 11110101, 10110011] -> little endian = [10110011, 11110101, 00101111]
 * Offset: 12 -> [10110011, 1111~0~101, 00101111]
 * Length: 8 -> [0010~1111~, 1111~0101, 1011~0011]
 * -> 010110110011
 */

const int NUM_MESSAGE_OPERATORS = 7;
const Message_operator message_operators[] = {
    // define your operators here
    {.name = "rpm", .message_id = 0x0A5, .mult_factor = 0.25, .add_factor = 0, .offset = 5, .length = 2, .bit_mode = false, .has_mask = 0, .mask = 0x0, .is_little_endian = true, .is_signed = false},
    {.name = "accelerator_pedal", .message_id = 0x0D9, .mult_factor = (1 / 40.0), .add_factor = 0, .offset = 16, .length = 12, .bit_mode = true, .has_mask = false, .mask = 0, .is_little_endian = true, .is_signed = true},
    {.name = "brake", .message_id = 0x0EF, .mult_factor = -2.0, .add_factor = 250.0, .offset = 3, .length = 1, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = true},
    {.name = "speed_mph", .message_id = 0x1A1, .mult_factor = 0.015625 / 1.60934, .add_factor = 0, .offset = 2, .length = 2, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = false},
    {.name = "gear", .message_id = 0x0F3, .mult_factor = 1, .add_factor = 0, .offset = 5, .length = 1, .bit_mode = false, .has_mask = true, .mask = 0xf, .is_little_endian = true, .is_signed = true},
    {.name = "Steering", .message_id = 0x302, .mult_factor = -1.0 / 22.75, .add_factor = 1440, .offset = 2, .length = 2, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = false},
    {.name = "Steering2", .message_id = 0x302, .mult_factor = 0.039555, .add_factor = -1296, .offset = 2, .length = 2, .bit_mode = false, .has_mask = false, .mask = 0x0, .is_little_endian = true, .is_signed = false},
};

/**
 * Acceptance Mask
 * 0x1A1, 0x0A5, 0x302, 0x0EF, 0x0D9
 *
 * 00010100101
 * 01100000010
 * 00011101111
 * 00011011001
 *
 *=01111111111
 *=0x3FF
 *
 */