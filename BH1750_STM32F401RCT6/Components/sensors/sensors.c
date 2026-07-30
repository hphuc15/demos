#include "sensors.h"
#include "bh1750.h"
#include "i2c.h"
#include "main.h"
#include "stm32f4xx_hal.h"

#include "utilities.h"

static BH1750_Dev bh1750;

/* HAL Layer */

/** @brief HAL i2c_read */
static int hal_i2c1_read(uint8_t addr, uint8_t *data, size_t len){
    HAL_StatusTypeDef s;
    s = HAL_I2C_Master_Receive(&hi2c1, addr << 1, data, (uint16_t)len, 100);
    return (s == HAL_OK) ? 0 : -1;
}

/** @brief HAL i2c_write */
static int hal_i2c1_write(uint8_t addr, const uint8_t *data, size_t len){
    HAL_StatusTypeDef s;
    s = HAL_I2C_Master_Transmit(&hi2c1, addr << 1, (uint8_t *)data, (uint16_t)len, 100);
    return (s == HAL_OK) ? 0 : -1;
}

/** @brief HAL delay_ms */
static void hal_delay_ms(uint32_t ms){
    HAL_Delay(ms);
}

/* Static each sensors initialize */

/** @brief Initialize bh1750 sensor */
static int bh1750_init(void){
    bh1750.i2c_addr = BH1750_I2C_ADDR_LOW;
    bh1750.i2c_read = hal_i2c1_read;
    bh1750.i2c_write = hal_i2c1_write;
    bh1750.delay_ms = hal_delay_ms;

    BH1750_Status s;
    s = BH1750_Init(&bh1750, BH1750_MODE_CONT_H_RES);
    return (s == BH1750_OK) ? 0 : -1;
}

/** @brief Read bh1750 data */
static int bh1750_read(sensors_data *data){
    BH1750_Status s;
    s = BH1750_ReadLux(&bh1750, &data->light_lux);
    return (s == BH1750_OK) ? 0 : -1;
}

/* PUBLIC APIs */

int sensors_init(void){
    if(bh1750_init() != 0){
        return -1;
    }
    
    return 0;
}

int sensors_read(sensors_data *data){
    int e = 0;
    e += bh1750_read(data);
    return (e == 0) ? 0 : -1;
}

int sensors_log(sensors_data *data){
    LOG_INFO("[SENSORS]:\tBH1750\tLight:\t%.2f\tlux", data->light_lux);
    return 0;
}