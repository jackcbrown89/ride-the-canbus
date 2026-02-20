#ifndef BLE_SERVICE_H
#define BLE_SERVICE_H

#include <stdint.h>

// UUID declarations
extern const uint16_t primary_service_uuid;
extern const uint16_t character_declaration_uuid;
extern const uint16_t character_client_config_uuid;
extern const uint8_t char_prop_read_notify;

// Initialize BLE functionality
void ble_init(void);

// Function to log a data point (already defined in main.c but modified for BLE)
void log_datapoint(const char name[20], int64_t value);

#endif /* BLE_SERVICE_H */