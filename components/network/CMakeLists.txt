idf_component_register(
    SRCS "network.c" "transport/http/http_transport.c" "transport/mqtt/mqtt_transport.c"
    INCLUDE_DIRS "include" "transport/http" "transport/mqtt"
    REQUIRES esp_http_client mqtt mbedtls WiFiManager
)