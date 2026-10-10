idf_component_register(SRCS "main.cpp"
                       INCLUDE_DIRS "."
                       REQUIRES esp_adc esp_wifi nvs_flash esp_event esp_netif)