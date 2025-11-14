#include "switch_control.h"
#include "config.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

volatile bool filter_enabled = true;

void switch_monitor_task(void *arg)
{
    gpio_config_t io_conf = {
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pin_bit_mask = 1ULL << SWITCH_GPIO
    };
    gpio_config(&io_conf);

    bool last_state = gpio_get_level(SWITCH_GPIO);

    for (;;)
    {
        bool state = gpio_get_level(SWITCH_GPIO);
        if (state == 0 && last_state == 1)
        {
            filter_enabled = !filter_enabled;
            ESP_LOGW(TAG, "Filtre %s", filter_enabled ? "ACTIVATE" : "DESACTIVATE");
        }
        last_state = state;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
