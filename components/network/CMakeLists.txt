idf_component_register(
    SRCS
        "network.c" "network_config.c" "wifi/wifi.c" "transport/http/http_transport.c" "transport/mqtt/mqtt_transport.c"
    INCLUDE_DIRS
        "include"
        "wifi"
        "transport/http" "transport/mqtt"
    REQUIRES
        esp_http_client mqtt mbedtls nvs_flash
        WiFiManager
)