#include "app.h"
#include "sensors.h"
#include "network.h"
#include "wifi_button.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static SensorsData_t s_data = {0};
static SemaphoreHandle_t s_data_mutex = NULL;

static void wifi_config_task(void *arg) {
    network_reconfigure();
    vTaskDelete(NULL);
}

static void on_btn_hold(void) {
    xTaskCreate(wifi_config_task, "wifi_cfg", 4096, NULL, 5, NULL);
}

void sensors_task(void *args){
    while(1){
        sensors_read(&s_data);
        sensors_log(&s_data);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void network_task(void *args){
    char payload[HTTP_PAYLOAD_SIZE];
    while(1){
        if(network_is_ready()){
            if(sensors_build_payload(&s_data, payload, sizeof(payload))){
                network_publish(payload);
            } else {
                ESP_LOGW("APP", "Failed to build payload");
            }
        } else {
            ESP_LOGD("APP", "Network not ready, skip");
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_init(void){
    sensors_init();
    network_init(NETWORK_PROTO_HTTP);
    wifi_button_init(on_btn_hold);

    xTaskCreate(sensors_task, "SENSORS_TASK", 2048, NULL, 5, NULL);
    xTaskCreate(network_task, "NETWORK_TASK", 4096, NULL, 5, NULL);
}

void delay_ms(uint32_t ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
}