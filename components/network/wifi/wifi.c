#include "wifi.h"
#include "WiFiManager.h"

static WiFiManager_t s_wm = {
    .ap_config = {
        .ssid = "2SL_Demo_BH1750",
        .password = "2SL_Demo_BH1750",
        .max_connection = 2,
        .authmode = WIFI_AUTH_WPA_WPA2_PSK
    }
};

/* State */
bool s_wifi_ready = false;

/* External connected callback */
static wifi_cb_t s_on_wifi_connected = NULL;
/* External disconnected callback */
static wifi_cb_t s_on_wifi_disconnected = NULL;

static void on_wifi_connected(){
    s_wifi_ready = true;
    if(s_on_wifi_connected){
        s_on_wifi_connected();
    }
}

static void on_wifi_disconnected(){
    s_wifi_ready = false;
    if(s_on_wifi_disconnected){
        s_on_wifi_disconnected();
    }
}

/* Public APIs */

void wifi_init(void) {
    s_wm.ConnectedAP_Cb = on_wifi_connected;
    s_wm.DisconnectedAP_Cb = on_wifi_disconnected;
    s_wm.sta_retry_num = 5;
    WiFiManagerPage_Init(&s_wm);
    WiFiManagerPage_AddParam(&s_wm, "host", "Host", "e.g. broker.example.com", "", "text", true);
    WiFiManagerPage_AddParam(&s_wm, "port", "Port", "e.g. 8883", "", "number", true);
    WiFiManagerPage_AddParam(&s_wm, "token", "Token", "Bearer token", "", "password", true);
    WiFiManager_Init(&s_wm);
}

void wifi_connect(void){
    WiFiManager_AutoConnect(&s_wm);
}

void wifi_stop(void){
    WiFiManager_Stop(&s_wm);
    s_wifi_ready = false;
}

void wifi_set_connected_cb(wifi_cb_t cb){
    s_on_wifi_connected = cb;
}

void wifi_set_disconnected_cb(wifi_cb_t cb){
    s_on_wifi_disconnected = cb;
}

bool wifi_is_ready(void){
    return s_wifi_ready;
}