typedef struct {
    char name[20]; 
    int message_id;
    float mult_factor;
    float add_factor; 
    int offset; 
    int length;
    bool bit_mode;
} Message_operator; 

/**
 * "Bit mode" example
 * [0x2F, 0xF5, 0xA3] = [00101111, 11110101, 10110011] -> little endian = [10110011, 11110101, 00101111]
 * Offset: 12 -> [10110011, 1111~0~101, 00101111]
 * Length: 8 -> [0010~1111~, 1111~0101, 1011~0011]
 * -> 010110110011
 */


const int NUM_MESSAGE_OPERATORS = 2;
const Message_operator message_operators[] = {
    // define your operators here
    // {"RPM", 0x0A5, 0.25, 0, 5, 2, false},
    {"Engine Torque", 0x0A5, 0.5, -1023.5, 16, 12, true},
    // {"Steering1", 0x301, -1/22.75, 1440, 2, 2, false},
    // {"Steering2", 0x302, -1/22.75, 1440, 2, 2, false},
    // {"Brake %", 0x0EF, -2, 250, 3, 1, false},
    // {"Accelerator pedal %", 0x0D9, (1 / 40.0), 0, 16, 12, true},
    // {"Fuel Range km", 0x330, 1 / 16.0, 0, 48, 12, true},
    // {"Gear", 0x0F3, 1, -4, 16, 4, true},
    {"Speed mph", 0x1A1, 0.015625 / 1.60934, 0, 2, 2, false}
};

/**
 * Acceptance Mask
 * 0x0A5, 0x302, 0x0EF, 0x0D9
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