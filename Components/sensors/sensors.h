#pragma once

#include <stdint.h>

typedef struct {
    float light_lux;
} sensors_data;

/** @brief Initialize sensors. @return 0 on success, other on fail. */
int sensors_init(void);

/**
 * @brief Read sensors value to sensors_data struct.
 * @param data sensors data buffer struct.
 * @return 0 on success, others on fail.
 */
int sensors_read(sensors_data *data);

/**
 * @brief Log sensors value.
 * @param data sensors data buffer struct.
 * @return 0 on success, others on fail.
 */
int sensors_log(sensors_data *data);