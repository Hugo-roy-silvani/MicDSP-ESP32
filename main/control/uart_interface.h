#pragma once

#include "driver/uart.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdint.h>

#include "expander.h"
#include "compressor.h"
#include "limiter.h"
#include "rms.h"
#include "iir_filter.h"
#include "fft.h"


typedef struct {
    expander_t *expd;
    compressor_t *comp;
    limiter_t *limiter;
    rms_filter_t *rms_out;
    eq3band_t *eq;
} dsp_context_t;

void uart_interface_init(void);
void uart_interface_task_ui(void *arg);
void uart_sendf(const char *fmt, ...);
void telemetry_task(void *arg);

