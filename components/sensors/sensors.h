#ifndef SENSORS_H
#define SENSORS_H

#include <stddef.h>
#include <stdbool.h>

typedef struct {
    float light_lux;
    bool has_lux;
} SensorsData_t;

void sensors_init(void);
void sensors_read(SensorsData_t *out);
void sensors_log(const SensorsData_t *data);

#endif /* SENSORS_H */