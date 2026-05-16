#include "network.h"
#include "http_transport.h"
#include "mqtt_transport.h"
#include "wifi.h"
/* ESP-IDF libs */
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"
#include "esp_err.h"
/* Standard libs */
#include <string.h>

/* LOG TAG */

// static const char *TAG              = "[NETWORK]";
static const char *TAG_WIFI         = "[NETWORK][WIFI]";
static const char *TAG_TRANSPORT    = "[NETWORK][TRANSPORT]";

/* STATE */
static SemaphoreHandle_t s_publish_mutex = NULL;
static network_proto_t s_proto = NETWORK_PROTO_HTTP;
static bool s_transport_up = false;

/* TRANSPORT VTABLE*/
typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*publish)(const char *payload);
    bool (*is_ready)(void);
    esp_err_t (*stop)(void);
} transport_ops_t;

static const transport_ops_t s_transports[] = {
    [NETWORK_PROTO_HTTP] = {
        .init = http_transport_init,
        .publish = http_transport_publish,
        .is_ready = http_transport_is_ready,
        .stop = http_transport_stop,
    },
    [NETWORK_PROTO_MQTT] = {
        .init = mqtt_transport_init,
        .publish = mqtt_transport_publish,
        .is_ready = mqtt_transport_is_ready,
        .stop = mqtt_transport_stop,
    },
};

static const transport_ops_t *s_active = NULL;

/* WIFI CALLBACKs */

/**
 * @brief WiFi connected callback
 * Use to set network application
 */
static void network_wifi_connected_cb(void) {
    ESP_LOGI(TAG_WIFI, "Connected");

    if (s_active && !s_transport_up) {
        esp_err_t err = s_active->init();
        if (err == ESP_OK) {
            s_transport_up = true;
            ESP_LOGI(TAG_TRANSPORT, "Transport up");
        } else {
            ESP_LOGE(TAG_TRANSPORT, "Init failed: %s", esp_err_to_name(err));
        }
    }
}

/**
 * @brief WiFi disconnected callback
 * Use to set network application
 */
static void network_wifi_disconnected_cb(void) {
    ESP_LOGW(TAG_WIFI, "Disconnected");
    xSemaphoreTake(s_publish_mutex, pdMS_TO_TICKS(6000)); // > timeout HTTP (5000ms)
    s_transport_up = false;
    xSemaphoreGive(s_publish_mutex);
}

/* PUBLIC APIs */

network_err_t network_init(network_proto_t proto) {
    if (proto >= sizeof(s_transports) / sizeof(s_transports[0])) {
        return NETWORK_ERR_INVALID_ARG;
    }
    s_publish_mutex = xSemaphoreCreateMutex();
    s_proto = proto;
    s_active = &s_transports[proto];

    ESP_LOGI(TAG_TRANSPORT, "Proto: %s", proto == NETWORK_PROTO_MQTT ? "MQTTS" : "HTTPS");

    /* WiFi Setup */
    wifi_set_connected_cb(network_wifi_connected_cb);
    wifi_set_disconnected_cb(network_wifi_disconnected_cb);
    wifi_init();
    wifi_connect();

    return NETWORK_OK;
}

network_err_t network_publish(const char *payload) {
    if (!s_active || !s_transport_up) {
        return NETWORK_ERR_INVALID_STATE;
    }

    if (xSemaphoreTake(s_publish_mutex, pdMS_TO_TICKS(6000)) != pdTRUE) {
        return NETWORK_ERR_INVALID_STATE;
    }

    esp_err_t err = ESP_ERR_INVALID_STATE;
    if (s_transport_up && s_active) {          // check lại sau khi có mutex
        err = s_active->publish(payload);
    }

    xSemaphoreGive(s_publish_mutex);
    switch (err) {
        case ESP_OK:                return NETWORK_OK;
        case ESP_ERR_INVALID_ARG:   return NETWORK_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE: return NETWORK_ERR_INVALID_STATE;
        default:                    return NETWORK_ERR_TRANSPORT;
    }
}


bool network_is_ready(void) {
    return wifi_is_ready() && s_active && s_active->is_ready();
}

network_err_t network_stop(void) {
    if (!s_active) {
        return NETWORK_OK;
    }
    
    xSemaphoreTake(s_publish_mutex, pdMS_TO_TICKS(6000));
    s_transport_up = false;
    esp_err_t err = s_active->stop();
    s_active = NULL;
    xSemaphoreGive(s_publish_mutex);

    wifi_stop();

    switch (err) {
        case ESP_OK:                return NETWORK_OK;
        case ESP_ERR_INVALID_ARG:   return NETWORK_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE: return NETWORK_ERR_INVALID_STATE;
        default:                    return NETWORK_ERR_TRANSPORT;
    }
}

network_err_t network_reconfigure(void) {
    xSemaphoreTake(s_publish_mutex, pdMS_TO_TICKS(6000));
    s_transport_up = false;
    if (s_active) s_active->stop();
    xSemaphoreGive(s_publish_mutex);

    wifi_stop();
    wifi_config();
    return NETWORK_OK;
}