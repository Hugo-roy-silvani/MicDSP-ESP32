#include "esp_dsp.h"
#include "uart_interface.h"
#include <math.h>

#define FFT_SIZE 512

extern float fft_last_bands[8];

void fft_init(void);
void analyze_fft_and_send(const float *samples);