idf_component_register(
    SRCS
        "main.c"

        "dsp/fft.c"
        "dsp/expander.c"
        "dsp/compressor.c"
        "dsp/iir_filter.c"
        "dsp/rms.c"
        "dsp/limiter.c"

        "audio_io/i2s_manager.c"

        "control/switch_control.c"
        "control/uart_interface.c"

    INCLUDE_DIRS
        "."
        "dsp"
        "audio_io"
        "control"
)