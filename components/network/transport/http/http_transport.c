// http_transport.c
#include "http_transport.h"
#include "network_config.h"

#include "esp_http_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

#define TAG "HTTP_TRANSPORT"

static bool s_ready = false;

esp_err_t http_transport_init(void) {
    // HTTP không cần kết nối trước, sẵn sàng ngay
    s_ready = true;
    ESP_LOGI(TAG, "Ready → %s", HTTP_TELEMETRY_URL);
    return ESP_OK;
}

esp_err_t http_transport_publish(const char *topic, const char *payload) {
    // topic bị bỏ qua với HTTP/ThingsBoard (URL cố định)
    // giữ tham số để API đồng nhất với MQTT
    (void)topic;

    if (!s_ready) return ESP_ERR_INVALID_STATE;
    if (!payload)  return ESP_ERR_INVALID_ARG;

    esp_http_client_config_t config = {
        .url            = HTTP_TELEMETRY_URL,
        .method         = HTTP_METHOD_POST,
        .crt_bundle_attach = esp_crt_bundle_attach,         /* TLS bundle by default */
        .timeout_ms     = 10000,
    };

    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (!client) return ESP_FAIL;

    esp_http_client_set_header(client, "Content-Type", "application/json");
    esp_http_client_set_post_field(client, payload, (int)strlen(payload));

    esp_err_t ret = esp_http_client_perform(client);
    if (ret == ESP_OK) {
        int status = esp_http_client_get_status_code(client);
        if (status != 200) {
            ESP_LOGW(TAG, "HTTP %d", status);
            ret = ESP_FAIL;
        } else {
            ESP_LOGD(TAG, "Published %d bytes", strlen(payload));
        }
    } else {
        ESP_LOGE(TAG, "perform failed: %s", esp_err_to_name(ret));
    }

    esp_http_client_cleanup(client);
    return ret;
}

bool http_transport_is_ready(void) {
    return s_ready;
}

esp_err_t http_transport_stop(void) {
    s_ready = false;
    return ESP_OK;
}