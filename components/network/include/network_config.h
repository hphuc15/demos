#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

/* Server Credentials */
#define SERVER_DEFAULT_HOST             "<default_server_host>"
#define SERVER_DEFAULT_PORT             <default_server_port>     /* 443 for MQTTS, 1883 for MQTT, 8883 for MQTTS*/
#define DEVICE_TOKEN_DEFAULT            "<default_device_token>"  /* BH1750_DEMO for Sensor Lab ThingsBoard server in this demo */
/* Network NVS Credentials */
#define NETWORK_CONFIG_NVS_NAMESPACE    "network_nvs"
#define NETWORK_CONFIG_NVS_HOST_KEY     "host"
#define NETWORK_CONFIG_NVS_PORT_KEY     "port"
#define NETWORK_CONFIG_NVS_TOKEN_KEY    "token"
/* ESP32 WiFi AP Credentials */
#define NETWORK_CONFIG_WIFI_AP_SSID     "BH1750_DEMO"
#define NETWORK_CONFIG_WIFI_AP_PASSWORD "BH1750_DEMO"

#endif /* NETWORK_CONFIG_H */