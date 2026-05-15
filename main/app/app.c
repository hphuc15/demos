#include "app.h"
#include "sensors.h"
#include "network.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static SensorsData_t s_data = {0};

static void app_task(void *args){
    while(1){
        sensors_read(&s_data);
        sensors_log(&s_data);
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_init(void){
    sensors_init();
    xTaskCreate(app_task, "APP_TASK", 2048, NULL, 5, NULL);
}

/* UTILITIES */

void delay_ms(uint32_t ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
}