#include "uart_interface.h"
#include "esp_log.h"
#include <string.h>
#include <stdlib.h>
#include "iir_filter.h"
#include <stdarg.h>
#include "freertos/semphr.h"

#define UART_PORT       UART_NUM_0
#define UART_BAUDRATE   115200
#define UART_BUF_SIZE   128

static const char *TAG = "UART_IF";
float fc_current =1000.0f;
float q_current = 0.707f;
filter_type_t type_current = FILTER_LOW_PASS;
const eq_band_t *low, *mid, *high;


static SemaphoreHandle_t uart_tx_mutex = NULL;

void uart_interface_init(void)
{
    const uart_config_t uart_config = {
        .baud_rate = UART_BAUDRATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
    };

    if (!uart_tx_mutex)
        uart_tx_mutex = xSemaphoreCreateMutex();

    ESP_ERROR_CHECK(uart_param_config(UART_PORT, &uart_config));
    ESP_ERROR_CHECK(uart_driver_install(UART_PORT, UART_BUF_SIZE * 2, 0, 0, NULL, 0));

    ESP_LOGI(TAG, "UART initialized at %d baud", UART_BAUDRATE);
}

void uart_sendf(const char *fmt, ...)
{
    if (fmt == NULL) return;

    char buf[128];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(buf, sizeof(buf), fmt, ap);
    va_end(ap);

    if (n <= 0) return;
    if (n >= (int)sizeof(buf)) n = (int)sizeof(buf) - 1;

    xSemaphoreTake(uart_tx_mutex, portMAX_DELAY);
    uart_write_bytes(UART_PORT, buf, n);
    uart_wait_tx_done(UART_PORT, pdMS_TO_TICKS(100));
    xSemaphoreGive(uart_tx_mutex);
}

void telemetry_task(void *arg)
{
    dsp_context_t *ctx = (dsp_context_t *)arg;   
    const int delay_ms = 100; // send frequency (5 Hz)
    while (1)
    {
        float rms_db = rms_get_dbfs(ctx->rms_out);

        // Generate message for GUI
        char msg[256];
        int len = snprintf(msg, sizeof(msg),
                           "STREAM:RMS=%.1f,FFT=%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f,%.1f\r\n",
                           rms_db,
                           fft_last_bands[0], fft_last_bands[1], fft_last_bands[2], fft_last_bands[3],
                           fft_last_bands[4], fft_last_bands[5], fft_last_bands[6], fft_last_bands[7]);

        uart_write_bytes(UART_PORT, msg, len);

        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

void uart_interface_task_ui(void *arg)
{
    dsp_context_t *ctx = (dsp_context_t *)arg;
    static char cmd_buf[128];
    size_t idx = 0;
    uint8_t rx[64];

    const char *prompt = "> ";
    uart_sendf("%s", prompt);

    expander_t   *expd   = ctx->expd;
    compressor_t *comp   = ctx->comp;
    limiter_t    *limitr = ctx->limiter;
    rms_filter_t *rmsout = ctx->rms_out;

    for (;;)
    {
        int len = uart_read_bytes(UART_PORT, rx, sizeof(rx), pdMS_TO_TICKS(50));
        for (int i = 0; i < len; i++)
        {
            uint8_t c = rx[i];

            // Fin de ligne → exécuter la commande
            if (c == '\r' || c == '\n')
            {
                cmd_buf[idx] = '\0';

                if (idx > 0)
                {
                    // ---------------- HELP ----------------
                    if (strcasecmp(cmd_buf, "help") == 0) {
                        uart_sendf(
                            "\r\nCommands:\r\n"
                            "  help                       - show this help\r\n"
                            "  ping                       - check connection\r\n"
                            "  get eq                     - show EQ status\r\n"
                            "  REQ_RMS                    - read current RMS\r\n"
                            "  EQ_<LOW|MID|HIGH>_<FC|Q|GAIN>=<val>\r\n"
                            "  EXPANDER_<THRESHOLD|RATIO|ATTACK|RELEASE|HOLD>=<val>\r\n"
                            "  COMP_<THRESHOLD|RATIO|MAKEUP|ATTACK|RELEASE|KNEE>=<val>\r\n"
                            "  LIMIT_<THRESHOLD|ATTACK|RELEASE>=<val>\r\n\r\n");
                    }

                    // ---------------- PING ----------------
                    else if (strcasecmp(cmd_buf, "ping") == 0) {
                        uart_sendf("pong\r\n");
                    }

                    // ---------------- EQ ----------------
                    else if (strncasecmp(cmd_buf, "EQ_", 3) == 0) {
                        char band[8], param[8];
                        float value;
                        if (sscanf(cmd_buf, "EQ_%7[^_]_%7[^=]=%f", band, param, &value) == 3) {
                            eq_band_id_t id;
                            if      (strcasecmp(band, "LOW")  == 0) id = EQ_BAND_LOW;
                            else if (strcasecmp(band, "MID")  == 0) id = EQ_BAND_MID;
                            else if (strcasecmp(band, "HIGH") == 0) id = EQ_BAND_HIGH;
                            else { uart_sendf("Invalid EQ band\r\n"); goto end_prompt; }

                            eq_band_t *target = eq_get_band(id);
                            if (!target) { uart_sendf("EQ band not found\r\n"); goto end_prompt; }

                            if      (strcasecmp(param, "FC")   == 0) target->fc      = value;
                            else if (strcasecmp(param, "Q")    == 0) target->Q       = value;
                            else if (strcasecmp(param, "GAIN") == 0) target->gain_db = value;
                            else { uart_sendf("Invalid EQ param\r\n"); goto end_prompt; }

                            update_filter_coefficients_eq(target);
                            uart_sendf("OK EQ_%s_%s=%.2f\r\n", band, param, value);
                        }
                        else uart_sendf("Invalid EQ format\r\n");
                    }

                    // ---------------- EXPANDER ----------------
                    else if (strncasecmp(cmd_buf, "EXPANDER_", 9) == 0) {
                        char param[16];
                        float value;
                        if (sscanf(cmd_buf, "EXPANDER_%15[^=]=%f", param, &value) == 2) {
                            if      (strcasecmp(param, "THRESHOLD") == 0) expd->threshold     = value;
                            else if (strcasecmp(param, "RATIO")     == 0) expd->ratio         = value;
                            else if (strcasecmp(param, "ATTACK")    == 0) expd->attack_coeff  = value;
                            else if (strcasecmp(param, "RELEASE")   == 0) expd->release_coeff = value;
                            else if (strcasecmp(param, "HOLD")      == 0) expd->hold_time     = value;
                            else { uart_sendf("Invalid EXP param\r\n"); goto end_prompt; }
                            uart_sendf("OK EXPANDER\r\n");
                        } else uart_sendf("Invalid EXP format\r\n");
                    }

                    // ---------------- COMPRESSOR ----------------
                    else if (strncasecmp(cmd_buf, "COMP_", 5) == 0) {
                        char param[16];
                        float value;
                        if (sscanf(cmd_buf, "COMP_%15[^=]=%f", param, &value) == 2) {
                            if      (strcasecmp(param, "THRESHOLD") == 0) comp->threshold     = value;
                            else if (strcasecmp(param, "RATIO")     == 0) comp->ratio         = value;
                            else if (strcasecmp(param, "MAKEUP")    == 0) comp->makeup        = value;
                            else if (strcasecmp(param, "ATTACK")    == 0) comp->attack_coeff  = value;
                            else if (strcasecmp(param, "RELEASE")   == 0) comp->release_coeff = value;
                            else if (strcasecmp(param, "KNEE")      == 0) comp->knee_db       = value;
                            else { uart_sendf("Invalid COMP param\r\n"); goto end_prompt; }
                            uart_sendf("OK COMP\r\n");
                        } else uart_sendf("Invalid COMP format\r\n");
                    }

                    // ---------------- LIMITER ----------------
                    else if (strncasecmp(cmd_buf, "LIMIT_", 6) == 0) {
                        char param[16];
                        float value;
                        if (sscanf(cmd_buf, "LIMIT_%15[^=]=%f", param, &value) == 2) {
                            if      (strcasecmp(param, "THRESHOLD") == 0) limitr->threshold = value;
                            else if (strcasecmp(param, "ATTACK")    == 0) limitr->attack    = value;
                            else if (strcasecmp(param, "RELEASE")   == 0) limitr->release   = value;
                            else { uart_sendf("Invalid LIMIT param\r\n"); goto end_prompt; }
                            uart_sendf("OK LIMIT\r\n");
                        } else uart_sendf("Invalid LIMIT format\r\n");
                    }

                    // ---------------- RMS REQUEST ----------------
                    else if (strcasecmp(cmd_buf, "REQ_RMS") == 0) {
                        float db = rms_get_dbfs(rmsout);
                        uart_sendf("RMS_DB=%.1f\r\n", db);
                    }

                    // ---------------- UNKNOWN ----------------
                    else {
                        uart_sendf("Unknown command\r\n");
                    }
                }

            end_prompt:
                idx = 0;
                //uart_sendf("%s", prompt);
                continue;
            }

            // Backspace local (affichage uniquement sur terminal)
            if (c == 0x08 || c == 0x7F) {
                if (idx > 0) idx--;
                continue;
            }

            // Ignorer caractères de contrôle
            if (c < 0x20 || c == 0x7F) continue;

            // Ajout dans le buffer
            if (idx < sizeof(cmd_buf) - 1) {
                cmd_buf[idx++] = (char)c;
                // PAS d’écho vers le PC ici !
            }
        }
    }
}