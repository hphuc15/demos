// i2c.c
#include "i2c.h"
#include "esp_log.h"

#include <stdint.h>
#include <stddef.h>
#include <string.h>

static const char *TAG = "[I2C]";

/* Bus handle */
static i2c_master_bus_handle_t s_bus_handle;

/* I2C Device buffer */

typedef struct {
    uint16_t                addr;
    i2c_master_dev_handle_t handle;
} i2c_device_entry_t;

static i2c_device_entry_t s_devices[I2C_MAX_DEVICES];
static int s_device_count = 0;

static i2c_master_dev_handle_t i2c_get_device(uint8_t dev_addr) {
    for (int i = 0; i < s_device_count; i++) {
        if (s_devices[i].addr == dev_addr) {
            return s_devices[i].handle;
        }
    }
    ESP_LOGE(TAG, "Device 0x%02X not found", dev_addr);
    return NULL;
}

/* PUBLIC APIs */

esp_err_t i2c_init(void) {
    i2c_master_bus_config_t bus_config = {
        .clk_source                  = I2C_CLK_SRC_DEFAULT,
        .i2c_port                    = I2C_MASTER_NUM,
        .scl_io_num                  = I2C_MASTER_SCL_IO,
        .sda_io_num                  = I2C_MASTER_SDA_IO,
        .glitch_ignore_cnt           = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t ret = i2c_new_master_bus(&bus_config, &s_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %s", esp_err_to_name(ret));
        return ret;
    }

    memset(s_devices, 0, sizeof(s_devices));
    ESP_LOGI(TAG, "I2C bus initialized");
    return ESP_OK;
}

esp_err_t i2c_add_device(uint16_t dev_addr, uint32_t scl_speed_hz) {
    if (s_device_count >= I2C_MAX_DEVICES) {
        ESP_LOGE(TAG, "Device table full (max %d)", I2C_MAX_DEVICES);
        return ESP_ERR_NO_MEM;
    }

    /* Check if address exists */
    for (int i = 0; i < s_device_count; i++) {
        if (s_devices[i].addr == dev_addr) {
            ESP_LOGW(TAG, "Device 0x%02X already registered", dev_addr);
            return ESP_OK;
        }
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = dev_addr,
        .scl_speed_hz    = scl_speed_hz,
    };

    i2c_master_dev_handle_t handle;
    esp_err_t ret = i2c_master_bus_add_device(s_bus_handle, &dev_config, &handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Add device 0x%02X failed: %s", dev_addr, esp_err_to_name(ret));
        return ret;
    }

    s_devices[s_device_count].addr   = dev_addr;
    s_devices[s_device_count].handle = handle;
    s_device_count++;

    ESP_LOGI(TAG, "Device 0x%02X registered (%d/%d)", dev_addr, s_device_count, I2C_MAX_DEVICES);
    return ESP_OK;
}


/* I2C HAL */

int hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len) {
    esp_err_t err = i2c_master_transmit(i2c_get_device(addr), data, len, -1);
    return (err == ESP_OK) ? 0 : -1;
}

int hal_i2c_read(uint8_t addr, uint8_t *data, size_t len) {
    esp_err_t err = i2c_master_receive(i2c_get_device(addr), data, len, -1);
    return (err == ESP_OK) ? 0 : -1;
}