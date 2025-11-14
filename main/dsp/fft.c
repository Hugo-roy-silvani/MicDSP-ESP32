#include "fft.h"
#include <config.h>


static float fft_data[2 * FFT_SIZE] ;
static float window[FFT_SIZE] ;

float fft_last_bands[8] = {0};

void fft_init(void)
{
    esp_err_t ret = dsps_fft2r_init_fc32(NULL, FFT_SIZE);
    if (ret != ESP_OK) {
        uart_sendf("FFT init failed: %d\r\n", ret);
    }

    dsps_wind_hann_f32(window, FFT_SIZE);

    for (int i = 0; i < 8; i++) {
        fft_last_bands[i] = -100.0f;  
    }
}

void analyze_fft_and_send(const float *samples)
{
    if (!samples) return;

    // Apply window
    for (int i = 0; i < FFT_SIZE; i++) {
        fft_data[2*i]   = samples[i] * window[i];
        fft_data[2*i+1] = 0.0f;
    }

    // FFT
    dsps_fft2r_fc32(fft_data, FFT_SIZE);
    dsps_bit_rev_fc32(fft_data, FFT_SIZE);

    // gain correction
    float win_sum = 0.0f;
    for (int i = 0; i < FFT_SIZE; i++) win_sum += window[i];
    float win_gain = win_sum / FFT_SIZE;

    //logarithmic bands
    const float band_edges[9] = {60, 120, 250, 500, 1000, 2000, 4000, 8000, 16000};

    for (int b = 0; b < 8; b++) {
        int start = (int)(band_edges[b]   * FFT_SIZE / I2S_SR);
        int end   = (int)(band_edges[b+1] * FFT_SIZE / I2S_SR);
        if (start < 1) start = 1;
        if (end > FFT_SIZE/2) end = FFT_SIZE/2;
        if (end <= start) end = start + 1;

        float acc = 0.0f;
        for (int i = start; i < end; i++) {
            float re = fft_data[2*i]   / win_gain;
            float im = fft_data[2*i+1] / win_gain;
            acc += re*re + im*im;
        }
        acc /= (float)(end - start);
        fft_last_bands[b] = 10.0f * log10f(acc + 1e-12f);
    }
}
