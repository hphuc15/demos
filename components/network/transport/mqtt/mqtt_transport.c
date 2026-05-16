// mqtt_transport.c
#include "mqtt_transport.h"
#include "network_config.h"

#include "mqtt_client.h"
#include "esp_crt_bundle.h"
#include "esp_log.h"

static const char *TAG_MQTT = "[NETWORK][MQTT]";
#define MQTT_PUBLISH_QOS 1

static esp_mqtt_client_handle_t s_client = NULL;
static bool s_ready = false;

static void mqtt_event_handler(void *arg, esp_event_base_t base, int32_t event_id, void *event_data) {
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

/* PUBLIC APIs */

esp_err_t mqtt_transport_init(void) {
    if (s_client)
        return ESP_OK;

    if (!network_config_exists())
    {
        ESP_LOGE(TAG_MQTT, "No config in NVS");
        return ESP_ERR_INVALID_STATE;
    }

    network_config_t cfg = {0};
    esp_err_t err = network_config_load(&cfg);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG_MQTT, "Load config failed: %s", esp_err_to_name(err));
        return err;
    }

    char uri[128];
    snprintf(uri, sizeof(uri), "mqtts://%s:%ld", cfg.host, cfg.port);

    esp_mqtt_client_config_t config = {
        .broker = {
            .address.uri = uri,
            .verification.crt_bundle_attach = esp_crt_bundle_attach,
        },
        .credentials = {
            .username = cfg.token,
            .authentication.password = "",
        },
        .session.keepalive = 60,
    };

    s_client = esp_mqtt_client_init(&config);
    if (!s_client){
        return ESP_FAIL;
    }

    esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);

    esp_err_t ret = esp_mqtt_client_start(s_client);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG_MQTT, "client_start failed: %s", esp_err_to_name(ret));
        esp_mqtt_client_destroy(s_client);
        s_client = NULL;
    }

    ESP_LOGI(TAG_MQTT, "Connecting → %s", uri);
    return ret;
}

esp_err_t mqtt_transport_publish(const char *topic, const char *payload) {
    if (!s_ready || !s_client){
        return ESP_ERR_INVALID_STATE;
    }
    if (!topic || !payload){
        return ESP_ERR_INVALID_ARG;
    }

    int msg_id = esp_mqtt_client_publish(s_client, topic, payload, 0, MQTT_PUBLISH_QOS, 0);
    if (msg_id < 0)
    {
        ESP_LOGE(TAG_MQTT, "Publish failed");
        return ESP_FAIL;
    }

    ESP_LOGD(TAG_MQTT, "Published msg_id=%d topic=%s", msg_id, topic);
    return ESP_OK;
}

bool mqtt_transport_is_ready(void) {
    return s_ready;
}

esp_err_t mqtt_transport_stop(void) {
    if (!s_client){
        return ESP_OK;
    }

    s_ready = false;
    esp_mqtt_client_stop(s_client);
    esp_mqtt_client_destroy(s_client);
    s_client = NULL;
    return ESP_OK;
}