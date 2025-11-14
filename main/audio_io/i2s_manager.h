#pragma once
#include "esp_err.h"
#include "driver/i2s_std.h"


void i2s_init_rx(void);
void i2s_init_tx(void);

i2s_chan_handle_t get_rx_channel(void);
i2s_chan_handle_t get_tx_channel(void);