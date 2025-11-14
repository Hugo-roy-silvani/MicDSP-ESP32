#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_timer.h" 
#include "esp_dsp.h"

#include "config.h"
#include "i2s_manager.h"
#include "iir_filter.h"
#include "switch_control.h"
#include "uart_interface.h"
#include "rms.h"
#include "limiter.h"
#include "compressor.h"
#include "expander.h"
#include "fft.h"

extern volatile bool filter_enabled;
rms_filter_t rms_in, rms_out;
limiter_t limiter;
compressor_t comp;
expander_t expd;
eq_band_t hpf;

dsp_context_t dsp_ctx = {
    .expd = &expd,
    .comp = &comp,
    .limiter = &limiter,
    .rms_out = &rms_out
};

static void i2s_loopback_task(void *arg)
{
    dsp_context_t *ctx = (dsp_context_t *)arg;  // access to dsp context
    i2s_chan_handle_t rx_chan = get_rx_channel();
    i2s_chan_handle_t tx_chan = get_tx_channel();

    int32_t rx_buf[128];
    int16_t tx_buf[128];
    size_t bytes_read, bytes_written;
    static int block_count = 0;
    int64_t t_proc_start =0;
    float level =0;
    static float fft_buf[FFT_SIZE];
    static int fft_idx = 0;

    for (;;)
    { 

        //int64_t t_start = esp_timer_get_time(); // calculate DSP + DMA perf
      
        if (i2s_channel_read(rx_chan, rx_buf, sizeof(rx_buf), &bytes_read, portMAX_DELAY) == ESP_OK)
        {
            int samples = bytes_read / sizeof(int32_t);

            // calculate DSP perf
            /*
            block_count++;
            if (block_count % 100 == 0){
                t_proc_start = esp_timer_get_time();
            }
            */

            for (int i = 0; i < samples; i++)
            {
                
                float x = (float)(rx_buf[i] >> 8) / 8388608.0f;
                
                x *= 3.0f; // pre-grain
                
                if (!filter_enabled) {
                    // === BYPASS TOTAL ===
                    float y = x;                           
                    if (y > 1.0f) y = 1.0f;
                    if (y < -1.0f) y = -1.0f;
                    tx_buf[i] = (int16_t)(y * 32767.0f);                    
                }
                else{
                    
                    // --- DSP Pipeline  ---
                    float y = eq3band_process(x);
                    level = rms_process(ctx->rms_out, y);
                    y = expander_process(ctx->expd, y, level);
                    y = compressor_process(ctx->comp, y, level);
                    y = limiter_process(ctx->limiter, y, level);

                    y = tanhf(y); // soft clip

                    fft_buf[fft_idx++] = y;
                    if (fft_idx >= FFT_SIZE) {
                        analyze_fft_and_send(fft_buf);
                        fft_idx = 0;
                    }

                    tx_buf[i] = (int16_t)(y * 32767.0f);
                
                }
            }
            
              
            // calculate DSP perf 
            /*
            if(block_count % 100 == 0){
                int64_t t_proc_end = esp_timer_get_time();
                int64_t proc_us = t_proc_end - t_proc_start;
                ESP_LOGI("PERF", "DSP only: %lld us for %d samples (%.2f us/sample)",proc_us, samples, (float)proc_us / samples);

            }
            */

            i2s_channel_write(tx_chan, tx_buf, samples * sizeof(int16_t), &bytes_written, portMAX_DELAY);
            
            // calculate DSP + DMA perf
            /*
            int64_t t_end = esp_timer_get_time();
            int64_t elapsed_us = t_end - t_start;
            ESP_LOGI("LAT", "Bloc traité en %lld us (%.2f ms)", elapsed_us, elapsed_us / 1000.0f);
            */           
        }
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting microphone → amplifier loopback with IIR filter...");

    i2s_init_rx();
    i2s_init_tx();
    eq_init();
    rms_init(&rms_out, I2S_SR, 20.0f);
    uart_interface_init();
    limiter_init(&limiter, I2S_SR, 0.6f, 3.0f, 150.0f);
    compressor_init(&comp, I2S_SR, 0.3f, 4.0f, 4.0f, 10.0f, 120.0f, 6.0f);  
    expander_init(&expd, I2S_SR, 0.02f, 2.0f, 5.0f, 100.0f, 100.0f);   
    fft_init();

    xTaskCreatePinnedToCore(i2s_loopback_task, "i2s", 8192, &dsp_ctx, 10, NULL, 1); // core 1
    xTaskCreatePinnedToCore(uart_interface_task_ui, "uart", 4096, &dsp_ctx, 5, NULL, 0); // core 0
    xTaskCreatePinnedToCore(switch_monitor_task, "sw", 2048, NULL, 3, NULL, 0); // core 0
    xTaskCreatePinnedToCore(telemetry_task, "telemetry", 4096, &dsp_ctx, 6, NULL, 0);

}
