#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"

constexpr int POT_GPIO = 34;   // ESP32: 34, C6: 3

extern "C" void app_main(void)
{
    adc_unit_t unit;
    adc_channel_t channel;
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(POT_GPIO, &unit, &channel));

    adc_oneshot_unit_handle_t adc;
    adc_oneshot_unit_init_cfg_t unit_cfg = {};
    unit_cfg.unit_id = unit;
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&unit_cfg, &adc));

    adc_oneshot_chan_cfg_t chan_cfg = {};
    chan_cfg.atten = ADC_ATTEN_DB_12;
    chan_cfg.bitwidth = ADC_BITWIDTH_DEFAULT;
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc, channel, &chan_cfg));

    while (true) {
        int raw = 0;
        ESP_ERROR_CHECK(adc_oneshot_read(adc, channel, &raw));
        printf("raw=%d  procent=%.1f\n", raw, raw / 4095.0f * 100.0f);
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}