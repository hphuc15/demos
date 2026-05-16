#include "network_config.h"
#include "nvs_flash.h"
#include "esp_log.h"

static const char *TAG_CONFIG = "[NETWORL][CFG]";

esp_err_t network_config_save(const network_config_t *cfg) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NETWORK_CFG_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK){
        return err;
    }

    nvs_set_str(h, "host",  cfg->host);
    nvs_set_str(h, "token", cfg->token);
    nvs_set_i32(h, "port",  cfg->port);
    err = nvs_commit(h);
    nvs_close(h);

    ESP_LOGI(TAG_CONFIG, "Saved host=%s port=%d token=%s", cfg->host, cfg->port, cfg->token);
    return err;
}

esp_err_t network_config_load(network_config_t *cfg) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NETWORK_CFG_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK){
        return err;
    }

    size_t host_len  = sizeof(cfg->host);
    size_t token_len = sizeof(cfg->token);
    nvs_get_str(h, "host",  cfg->host,  &host_len);
    nvs_get_str(h, "token", cfg->token, &token_len);
    nvs_get_i32(h, "port",  &cfg->port);
    nvs_close(h);
    return ESP_OK;
}

bool network_config_exists(void) {
    nvs_handle_t h;
    if (nvs_open(NETWORK_CFG_NVS_NAMESPACE, NVS_READONLY, &h) != ESP_OK){
        return false;
    }

    char buf[4]; size_t len = sizeof(buf);
    bool ok = nvs_get_str(h, "token", buf, &len) == ESP_OK;
    nvs_close(h);
    return ok;
}