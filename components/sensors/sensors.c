// sensors.c
#include "sensors.h"
#include "bh1750.h"
#include "i2c.h"
#include "utilities.h"

#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


static const char *TAG      = "[SENSORS]";
static const char *TAG_LOG  = "[SENSORS][LOG]";


/* Utilities */
void hal_delay_ms(uint32_t ms){
    vTaskDelay(pdMS_TO_TICKS(ms));
}

/* BH1750 struct */
static BH1750_Dev s_bh1750 = {
    .i2c_addr  = BH1750_I2C_ADDR_LOW,
    .mode      = BH1750_MODE_CONT_H_RES,
    .delay_ms  = hal_delay_ms,
    .i2c_write = hal_i2c_write,
    .i2c_read  = hal_i2c_read,
};

/* Public API */

/**
 * @brief Initialize all sensors
 */
void sensors_init(void) {
    if (BH1750_Init(&s_bh1750, BH1750_MODE_CONT_H_RES) != BH1750_OK) {
        ESP_LOGE(TAG, "Failed to initialize BH1750");
    }
}

void sensors_read(SensorsData_t *out) {
    out->has_lux = (BH1750_ReadLux(&s_bh1750, &out->light_lux) == BH1750_OK);
}

void sensors_log(const SensorsData_t *data) {
    if (data->has_lux) {
        ESP_LOGI(TAG, "Light: %.2f lux", data->light_lux);
    }
}