#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>


/* ThingsBoard Cloud */
// #define TB_HOST         "thingsboard.cloud"
// #define TB_DEVICE_TOKEN "2SL_Demo_BH1750"

#define NETWORK_CFG_NVS_NAMESPACE    "network_cfg"

typedef struct {
    char host[64];
    char token[64];
    int32_t port;
} network_config_t;

esp_err_t network_config_save(const network_config_t *cfg);
esp_err_t network_config_load(network_config_t *cfg);
bool      network_config_exists(void);


#endif /* NETWORK_CONFIG_H */
