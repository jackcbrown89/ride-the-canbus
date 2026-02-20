#ifndef OPERATORS_H
#define OPERATORS_H

#include <stdint.h>
#include <stdbool.h>

// Structure for message operators
typedef struct
{
    int message_id;    // CAN message ID
    char name[20];     // Name for the data point
    int offset;        // Start byte or bit position
    int length;        // Length in bytes or bits
    float mult_factor; // Multiplication factor
    float add_factor;  // Addition factor
    bool is_signed;    // Whether the value is signed
    bool bit_mode;     // Whether to operate on bits rather than bytes
    bool is_little_endian;
    bool has_mask; // Whether to apply a mask
    int64_t mask;  // Mask to apply if has_mask is true
} message_operator_t;

// Number of defined message operators
#define NUM_MESSAGE_OPERATORS 7

// Array of message operators
extern const message_operator_t message_operators[NUM_MESSAGE_OPERATORS];

#endif /* OPERATORS_H */