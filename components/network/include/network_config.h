#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

/* Server Credentials */
#define SERVER_DEFAULT_HOST             "<default_server_host>"
#define SERVER_DEFAULT_PORT             <default_server_port>
#define DEVICE_TOKEN_DEFAULT            "<default_device_token>"
/* Network NVS Credentials */
#define NETWORK_CONFIG_NVS_NAMESPACE    "<network_nvs_namespace>"
#define NETWORK_CONFIG_NVS_HOST_KEY     "<network_nvs_host_key"
#define NETWORK_CONFIG_NVS_PORT_KEY     "<network_nvs_port_key>"
#define NETWORK_CONFIG_NVS_TOKEN_KEY    "<network_nvs_token_key>"
/* ESP32 WiFi AP Credentials */
#define NETWORK_CONFIG_WIFI_AP_SSID     "<network_ap_ssid>"
#define NETWORK_CONFIG_WIFI_AP_PASSWORD "<network_ap_password>"

#endif /* NETWORK_CONFIG_H */