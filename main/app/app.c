#include "app.h"
#include "sensors.h"
#include "network.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static SensorsData_t s_data = {0};

static void app_task(void *args){
    char payload[HTTP_PAYLOAD_SIZE];
    while(1){
        sensors_read(&s_data);
        sensors_log(&s_data);
        if(network_is_ready()){
            if(sensors_build_payload(&s_data, payload, sizeof(payload))){
                network_publish(NULL, payload);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_init(void){
    sensors_init();
    network_init(NETWORK_PROTO_HTTP);
    xTaskCreate(app_task, "APP_TASK", 8192, NULL, 5, NULL);
}

/* UTILITIES */

void delay_ms(uint32_t ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
}