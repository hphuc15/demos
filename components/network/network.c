#include "network.h"
#include "http_transport.h"
#include "mqtt_transport.h"
#include "WiFiManager.h"
/* ESP-IDF libs */
#include "esp_log.h"
#include "esp_err.h"
/* Standard libs */
#include <string.h>

/* ------------------------------------------------------------------
 *  Logging
 * ------------------------------------------------------------------ */

static const char *TAG              = "[NETWORK]";
static const char *TAG_WIFI         = "[NETWORK][WIFI]";
static const char *TAG_TRANSPORT    = "[NETWORK][TRANSPORT]";

/* ------------------------------------------------------------------
 *  State
 * ------------------------------------------------------------------ */

static const transport_ops_t *s_active       = NULL;
static network_proto_t        s_proto        = NETWORK_PROTO_HTTP;
static bool                   s_wifi_ready   = false;
static bool                   s_transport_up = false;

/* ------------------------------------------------------------------
 *  Transport vtable
 * ------------------------------------------------------------------ */

typedef struct {
    esp_err_t (*init)(void);
    esp_err_t (*publish)(const char *topic, const char *payload);
    bool      (*is_ready)(void);
    esp_err_t (*stop)(void);
} transport_ops_t;

static const transport_ops_t s_transports[] = {
    [NETWORK_PROTO_HTTP] = {
        .init     = http_transport_init,
        .publish  = http_transport_publish,
        .is_ready = http_transport_is_ready,
        .stop     = http_transport_stop,
    },
    [NETWORK_PROTO_MQTT] = {
        .init     = mqtt_transport_init,
        .publish  = mqtt_transport_publish,
        .is_ready = mqtt_transport_is_ready,
        .stop     = mqtt_transport_stop,
    },
};

static const transport_ops_t *s_active = NULL;

/* ------------------------------------------------------------------
 *  WiFi callbacks
 * ------------------------------------------------------------------ */

static void s_on_wifi_connected(){
    ESP_LOGI(TAG_WIFI, "Connected");
    s_wifi_ready = true;

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

static void s_on_wifi_disconnected(){
    ESP_LOGW(TAG_WIFI, "Disconnected");
    s_wifi_ready = false;
    s_transport_up = false;
}


/* ------------------------------------------------------------------
 *  WiFi internal
 * ------------------------------------------------------------------ */

static WiFiManager_t s_wm = {
    .ap_config = {
        .ssid = "2SL_Demo_BH1750",
        .password = "2SL_Demo_BH1750",
        .authmode = WIFI_AUTH_WPA_WPA2_PSK,
        .max_connection = 2
    }
};

static void wifi_init(void){
    s_wm.ConnectedAP_Cb = s_on_wifi_connected;
    s_wm.DisconnectedAP_Cb = s_on_wifi_disconnected;
    s_wm.sta_retry_num = 5;
    WiFiManager_Init(&s_wm);
}

static void wifi_connect(void){
    WiFiManager_AutoConnect(&s_wm);
}

static void wifi_stop(void){
    WiFiManager_Stop(&s_wm);
    s_wifi_ready = false;
    s_transport_up = false;
}


/* ------------------------------------------------------------------
 *  Helpers
 * ------------------------------------------------------------------ */

static network_err_t from_esp(esp_err_t err) {
    switch (err) {
        case ESP_OK:                return NETWORK_OK;
        case ESP_ERR_INVALID_ARG:   return NETWORK_ERR_INVALID_ARG;
        case ESP_ERR_INVALID_STATE: return NETWORK_ERR_INVALID_STATE;
        default:                    return NETWORK_ERR_TRANSPORT;
    }
}

/* ------------------------------------------------------------------ */
/*  Public API                                                         */
/* ------------------------------------------------------------------ */

network_err_t network_init(network_proto_t proto)
{
    if (proto >= sizeof(s_transports) / sizeof(s_transports[0])) {
        return NETWORK_ERR_INVALID_ARG;
    }

    s_proto = proto;
    s_active = &s_transports[proto];

    ESP_LOGI(TAG_TRANSPORT, "Proto: %s", proto == NETWORK_PROTO_MQTT ? "MQTTS" : "HTTPS");

    wifi_init();
    wifi_start();

    return NETWORK_OK;
}

network_err_t network_publish(const char *topic, const char *payload) {
    if (!s_active || !s_transport_up) return NETWORK_ERR_INVALID_STATE;
    return from_esp(s_active->publish(topic, payload));
}

bool network_is_ready(void) {
    return s_active && s_active->is_ready();
}

network_err_t network_stop(void) {
    if (!s_active) return NETWORK_OK;
    network_err_t ret = from_esp(s_active->stop());
    s_active = NULL;
    s_transport_up = NULL;
    wifi_stop();
    return ret;
}