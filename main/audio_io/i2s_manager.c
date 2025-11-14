#include "i2s_manager.h"
#include "config.h"
#include "esp_log.h"
#include "driver/i2s_std.h"

static i2s_chan_handle_t rx_chan;
static i2s_chan_handle_t tx_chan;

i2s_chan_handle_t get_rx_channel(void) { return rx_chan; }
i2s_chan_handle_t get_tx_channel(void) { return tx_chan; }

void i2s_init_rx(void)
{
    ESP_LOGI(TAG, "I2S RX Initialisation...");
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT_RX, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = 128;   
    chan_cfg.dma_desc_num  = 3;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t cfg_rx = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SR),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode      = I2S_SLOT_MODE_MONO,
            .slot_mask      = I2S_STD_SLOT_LEFT,
            .ws_width       = I2S_DATA_BIT_WIDTH_32BIT,
            .bit_shift      = true,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = MIC_BCLK_GPIO,
            .ws   = MIC_LRCLK_GPIO,
            .din  = MIC_DIN_GPIO,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &cfg_rx));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

void i2s_init_tx(void)
{
    ESP_LOGI(TAG, "I2S TX Initialisation...");
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_PORT_TX, I2S_ROLE_MASTER);
    chan_cfg.dma_frame_num = 128;   
    chan_cfg.dma_desc_num  = 3;
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t cfg_tx = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(I2S_SR),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_16BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_16BIT,
            .slot_mode      = I2S_SLOT_MODE_MONO,
            .slot_mask      = I2S_STD_SLOT_LEFT,
            .ws_width       = I2S_DATA_BIT_WIDTH_16BIT,
            .bit_shift      = true,
        },
        .gpio_cfg = {
            .mclk = I2S_GPIO_UNUSED,
            .bclk = AMP_BCLK_GPIO,
            .ws   = AMP_LRCLK_GPIO,
            .dout = AMP_DOUT_GPIO,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &cfg_tx));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
}
