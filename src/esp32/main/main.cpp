#include <stdio.h>
#include <string.h>
#include <time.h>
#include "esp_sntp.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "nvs_flash.h"

static const char *TAG = "iot";
constexpr int POT_GPIO = 34;   // ESP32: 34, C6: 3
static volatile bool wifi_connected = false;

static void wifi_event_handler(void *, esp_event_base_t base, int32_t id, void *data)
{
    if (base == WIFI_EVENT && id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "[WiFi] connecting...");
        esp_wifi_connect();
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_CONNECTED) {
        ESP_LOGI(TAG, "[WiFi] associated with access point, waiting for IP");
    } else if (base == WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED) {
        auto *d = (wifi_event_sta_disconnected_t *)data;
        wifi_connected = false;
        ESP_LOGW(TAG, "[WiFi] disconnected, reason=%d, retrying", d->reason);
        esp_wifi_connect();
    } else if (base == IP_EVENT && id == IP_EVENT_STA_GOT_IP) {
        auto *e = (ip_event_got_ip_t *)data;
        ESP_LOGI(TAG, "[WiFi] connected, IP=" IPSTR, IP2STR(&e->ip_info.ip));
        wifi_connected = true;
    }
}

static void wifi_init()
{
    esp_err_t r = nvs_flash_init();
    if (r == ESP_ERR_NVS_NO_FREE_PAGES || r == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        r = nvs_flash_init();
    }
    ESP_ERROR_CHECK(r);

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wc = {};
    strncpy((char *)wc.sta.ssid, CONFIG_WIFI_SSID, sizeof(wc.sta.ssid) - 1);
    strncpy((char *)wc.sta.password, CONFIG_WIFI_PASSWORD, sizeof(wc.sta.password) - 1);
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wc));
    ESP_ERROR_CHECK(esp_wifi_start());
}

static esp_mqtt_client_handle_t mqtt_client = nullptr;
static volatile bool mqtt_connected = false;

static void mqtt_event_handler(void *, esp_event_base_t, int32_t id, void *)
{
    if (id == MQTT_EVENT_CONNECTED) {
        mqtt_connected = true;
        ESP_LOGI(TAG, "[MQTT] connected");
    } else if (id == MQTT_EVENT_DISCONNECTED) {
        mqtt_connected = false;
        ESP_LOGW(TAG, "[MQTT] disconnected, client reconnects automatically");
    } else if (id == MQTT_EVENT_ERROR) {
        ESP_LOGE(TAG, "[MQTT] error");
    }
}

extern const char ca_pem_start[] asm("_binary_ca_crt_start");

static void mqtt_init()
{
    esp_mqtt_client_config_t cfg = {};
    cfg.broker.address.uri = CONFIG_MQTT_BROKER_URI;
    cfg.credentials.username = CONFIG_MQTT_USERNAME;
    cfg.credentials.authentication.password = CONFIG_MQTT_PASSWORD;
    cfg.broker.verification.certificate = ca_pem_start;
    mqtt_client = esp_mqtt_client_init(&cfg);
    esp_mqtt_client_register_event(mqtt_client, MQTT_EVENT_ANY, mqtt_event_handler, nullptr);
    esp_mqtt_client_start(mqtt_client);
}

static void time_init()
{
    setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);   // Stockholm time with DST
    tzset();
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();
}

static bool iso_now(char *buf, size_t len)
{
    time_t now = time(nullptr);
    if (now < 1700000000) return false;          // clock not synced yet
    struct tm t;
    localtime_r(&now, &t);
    char base[24];
    strftime(base, sizeof(base), "%Y-%m-%dT%H:%M:%S", &t);
    char zone[8];
    strftime(zone, sizeof(zone), "%z", &t);      // e.g. +0200
    snprintf(buf, len, "%s%.3s:%s", base, zone, zone + 3);
    return true;
}

extern "C" void app_main(void)
{
    wifi_init();
    mqtt_init();
    time_init();

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
        float percent = raw / 4095.0f * 100.0f;

        char ts[40];
        if (mqtt_connected && iso_now(ts, sizeof(ts))) {
            char payload[160];
            snprintf(payload, sizeof(payload),
                     "{\"sensorId\":\"room-a-pot-01\",\"timestamp\":\"%s\",\"value\":%.1f,\"unit\":\"%%\"}",
                     ts, percent);
            esp_mqtt_client_publish(mqtt_client, "fastighet/rum-a/pot-01/data", payload, 0, 1, 0);
            ESP_LOGI(TAG, "[PUB] %s", payload);
        } else {
            ESP_LOGW(TAG, "waiting (mqtt=%s, wifi=%s, time synced=%s)",
                     mqtt_connected ? "ok" : "no", wifi_connected ? "ok" : "no",
                     time(nullptr) > 1700000000 ? "yes" : "no");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}