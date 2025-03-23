// SNIFFER_MODE

// const gpio_num_t TX_GPIO = GPIO_NUM_23;
// const gpio_num_t RX_GPIO = GPIO_NUM_15;

// twai_general_config_t SNIFFER_GENERAL_CONFIG = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_LISTEN_ONLY);
// twai_timing_config_t PT_CANBUS_TIMING_CONFIG = TWAI_TIMING_CONFIG_500KBITS();
// twai_filter_config_t PT_CANBUS_FILTER_CONFIG = {.acceptance_code = 0, .acceptance_mask =0xFFFFFFFF, .single_filter = true};

// ACK_MODE
const gpio_num_t TX_GPIO = GPIO_NUM_0;
const gpio_num_t RX_GPIO = GPIO_NUM_1;

twai_general_config_t SNIFFER_GENERAL_CONFIG = TWAI_GENERAL_CONFIG_DEFAULT(TX_GPIO, RX_GPIO, TWAI_MODE_NORMAL);
twai_timing_config_t PT_CANBUS_TIMING_CONFIG = TWAI_TIMING_CONFIG_500KBITS();
twai_filter_config_t PT_CANBUS_FILTER_CONFIG = {.acceptance_code = 0, .acceptance_mask = 0xFFFFFFFF, .single_filter = true};
