#include "sensors.h"
#include "network.h"

static SensorsData_t = {0};

void app_init(void){
    sensors_init();
    sensors_read();
}