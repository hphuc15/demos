#include "mqtt_transport.h"
#include "wifi.h"

#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

static const char *TAG_MQTT = "[NETWORK][MQTT]";

#define MQTT_PUBLISH_QOS 1

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_ready = false;

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                               int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t ev = event_data;
    switch (ev->event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG_MQTT, "Connected");
        s_ready = true;
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGW(TAG_MQTT, "Disconnected");
        s_ready = false;
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG_MQTT, "Error type: %d", ev->error_handle->error_type);
        break;
    default:
        break;
    }
}

esp_err_t mqtt_transport_init(void)
{
    if (s_client)
        return ESP_OK;

    const char *host  = wifi_get_host();
    uint32_t port  = wifi_get_port();
    const char *token = wifi_get_token();

    char uri[128];
    const bool use_tls = (port == 8883);
    snprintf(uri, sizeof(uri), "%s://%s:%ld", use_tls ? "mqtts" : "mqtt", host, port);

    esp_mqtt_client_config_t config = {
        .broker = {
            .address.uri = uri,
            .verification.crt_bundle_attach = esp_crt_bundle_attach,
        },
        .credentials = {
            .username = token,
            .authentication.password = "",
        },
        .session.keepalive = 60,
    };

    s_client = esp_mqtt_client_init(&config);
    if (!s_client){
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t err = esp_mqtt_client_start(s_client);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_MQTT, "client_start failed: %s", esp_err_to_name(err));
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
        return err;
    }

    ESP_LOGI(TAG_MQTT, "Connecting → %s", uri);
    return ESP_OK;
}

esp_err_t mqtt_transport_publish(const char *payload)
{
    if (!s_ready || !s_client){
        return ESP_ERR_INVALID_STATE;
    }
    if (!payload){
        return ESP_ERR_INVALID_ARG;
    }

    int msg_id = esp_mqtt_client_publish(s_client, "v1/devices/me/telemetry", payload, 0, MQTT_PUBLISH_QOS, 0);
    if (msg_id < 0){
        ESP_LOGE(TAG_MQTT, "Publish failed");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG_MQTT, "Published msg_id=%d", msg_id);
    return ESP_OK;
}

bool mqtt_transport_is_ready(void) { return s_ready; }

esp_err_t mqtt_transport_stop(void)
{
    if (!s_client){
        return ESP_OK;
    }

    s_ready = false;
    esp_mqtt_client_stop(s_client);
    esp_mqtt_client_destroy(s_client);
    s_client = NULL;
    return ESP_OK;
}