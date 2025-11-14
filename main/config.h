#pragma once
#include "driver/gpio.h"
#include "driver/i2s_std.h"

#define TAG "I2S_LOOPBACK_IIR"

// === Audio Parameters ===
#define I2S_SR             48000 // sampling frequency

// === Control Button ===
#define SWITCH_GPIO        GPIO_NUM_0

// === I2S RX (Mic) ===
#define I2S_PORT_RX        I2S_NUM_1
#define MIC_BCLK_GPIO      GPIO_NUM_14
#define MIC_LRCLK_GPIO     GPIO_NUM_15
#define MIC_DIN_GPIO       GPIO_NUM_32

// === I2S TX (Amplifier) ===
#define I2S_PORT_TX        I2S_NUM_0
#define AMP_BCLK_GPIO      GPIO_NUM_26
#define AMP_LRCLK_GPIO     GPIO_NUM_25
#define AMP_DOUT_GPIO      GPIO_NUM_22
