#include "http_transport.h"
#include "network_config.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#include <string.h>
#include <stdio.h>

static const char *TAG_HTTP = "[NETWORK][HTTP]";

static bool s_ready = false;
static char s_url[256] = {0};

esp_err_t http_transport_init(void) {
    if (!network_config_exists()) {
        ESP_LOGE(TAG_HTTP, "No config in NVS");
        return ESP_ERR_INVALID_STATE;
    }

    network_config_t cfg = {0};
    esp_err_t err = network_config_load(&cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG_HTTP, "Load config failed: %s", esp_err_to_name(err));
        return err;
    }

    snprintf(s_url, sizeof(s_url), "https://%s:%ld/api/v1/%s/telemetry", cfg.host, cfg.port, cfg.token);

    ESP_LOGI(TAG_HTTP, "Ready: %s", s_url);
    s_ready = true;
    return ESP_OK;
}

esp_err_t http_transport_publish(const char *topic, const char *payload) {
    (void)topic;

    if (!s_ready){
        return ESP_ERR_INVALID_STATE;
    }
    if (!payload){
        return ESP_ERR_INVALID_ARG;
    }

    esp_http_client_config_t config = {
        .url               = s_url,
        .method            = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,
        .timeout_ms        = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, (int)strlen(payload));

    esp_err_t ret = esp_http_client_perform(client);
    if (ret == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status != 200) {
            ESP_LOGW(TAG_HTTP, "HTTP %d", status);
            ret = ESP_FAIL;
        } else {
            ESP_LOGD(TAG_HTTP, "Published %d bytes", (int)strlen(payload));
        }
    } else {
        ESP_LOGE(TAG_HTTP, "perform failed: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    return ret;
}

bool http_transport_is_ready(void) {
    return s_ready;
}

esp_err_t http_transport_stop(void) {
    s_ready = false;
    s_url[0] = '\0';
    return ESP_OK;
}