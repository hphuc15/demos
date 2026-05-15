// i2c.h
#ifndef I2C_H
#define I2C_H

#include "driver/i2c_master.h"
#include "esp_err.h"

#define I2C_MASTER_NUM     I2C_NUM_0
#define I2C_MASTER_SCL_IO  22
#define I2C_MASTER_SDA_IO  21
#define I2C_MASTER_FREQ_HZ 400000
#define I2C_MAX_DEVICES    8

esp_err_t i2c_init(void);
esp_err_t i2c_add_device(uint16_t dev_addr, uint32_t scl_speed_hz);

/* I2C HAL */
int hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len);
int hal_i2c_read(uint8_t addr, uint8_t *data, size_t len);

#endif /* I2C_H */