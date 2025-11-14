# 🎧 MicDSP-ESP32 — Real-Time Microphone DSP Pipeline

*A modular real-time DSP pipeline running on ESP32, featuring EQ, dynamics processing, RMS/FFT analysis and a Python GUI for live control and visualization. Designed as a compact audio front-end for microphone input using the built-in ADC/DAC and I2S interface.*

---

## 🚀 Features

### 🎚️ **DSP Processing**
- 3-band IIR Equalizer
- Expander (noise reduction & gating)
- Compressor (dynamic range control)
- Limiter (anti-clipping protection)
- RMS envelope detection
- FFT spectrum analysis

*All algorithms run in real-time under FreeRTOS using a block-processing architecture.*

### 🎵 **Audio I/O**
- ESP32 internal ADC/DAC
- I2S input/output
- Circular DMA buffers
- Configurable sample rate (e.g. 16 kHz / 32 kHz / 48 kHz)

### 🖥️ **Control & Monitoring GUI**
*A Python/PyQt6 interface providing:*
- Real-time FFT display
- Adjustable EQ band gains
- Threshold/ratio/attack/release for dynamics modules
- Live RMS metering
- Serial (UART) communication with the ESP32

## Project Structure 

MicDSP-ESP32/  
│  
├── main/  
│   ├── main.c  
│   ├── config.h  
│   ├── dsp/  
│   │     ├── iir_filter.c/.h  
│   │     ├── compressor.c/.h  
│   │     ├── expander.c/.h  
│   │     ├── limiter.c/.h  
│   │     ├── rms.c/.h  
│   │     └── fft.c/.h  
│   ├── audio_io/  
│   │     └── i2s_manager.c/.h  
│   └── control/  
│         ├── switch_control.c/.h  
│         ├── uart_interface.c/.h  
│  
├── UI/                  
└── LICENSE  
## 🧩 Hardware Setup

| **Component**       | **Model**               | **Description**                          |
|---------------------|-------------------------|------------------------------------------|
| **Microphone**      | INMP441                 | I²S digital MEMS microphone              |
| **MCU**            | ESP32 DevKit             | Microcontroller board                    |
| **Amplifier**      | MAX98357A               | I²S class-D amplifier (3W)               |
| **Speaker**        | Passive loudspeaker     | 4Ω or 8Ω, compatible with MAX98357A      |

## Architecture overview

Microphone → ADC → I2S → DMA Buffer → DSP Pipeline → DAC → Amplifier → Speaker  
.                                          │  
.                                     UART Control  
.                                          │  
.                                      GUI (PyQt6)  
## ⚙️ Technologies Used
- ESP-IDF 5.x
- ESP-DSP library
- FreeRTOS (tasks & timing)
- C / C++
- I2S (ADC/DAC internal)
- PyQt6
- FFT, block-based DSP processing
  
---

## 🎤 Goals of the Project
- Build a real-time DSP audio front-end from scratch
- Understand low-level I2S, DMA, ADC/DAC timing
- Implement core DSP modules (EQ, compressor, limiter)
- Visualize and tune everything via a live GUI
- Provide a clean, modular architecture for future extensions

## 🛠️ Build Instructions

### Requirements
- ESP-IDF (≥ 5.0)
- Python 3.10+
- ESP32 DevKitC or equivalent

### Build
~~~bash
#Create projet
idf.py create-project "project name"

# Configure target and environment
idf.py set-target esp32

# Build
idf.py build

# Flash to device
idf.py flash

# Monitor serial output (Do not use if using GUI)
idf.py monitor
~~~
Run the GUI 
~~~bash
pip install -r requirements.txt
python gui.py
~~~
