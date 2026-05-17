idf_component_register(
    SRCS
        "network.c"
        "wifi/wifi.c" "wifi/WiFiManager/src/WiFiManager_core.c" "wifi/WiFiManager/src/WiFiManager_page.c" "wifi/WiFiManager/src/WiFiManager_portal.c"
        "transport/http/http_transport.c" "transport/mqtt/mqtt_transport.c"
    INCLUDE_DIRS
        "include"
        "wifi" "wifi/WiFiManager/include"
        "transport/http" "transport/mqtt"
    REQUIRES
        esp_http_client mqtt mbedtls nvs_flash driver
        esp_wifi nvs_flash esp_http_server esp_event
        hardware
)