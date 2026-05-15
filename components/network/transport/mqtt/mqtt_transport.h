// mqtt_transport.h
#ifndef MQTT_TRANSPORT_H
#define MQTT_TRANSPORT_H

#include "esp_err.h"
#include <stdbool.h>

esp_err_t mqtt_transport_init(void);
esp_err_t mqtt_transport_publish(const char *topic, const char *payload);
bool      mqtt_transport_is_ready(void);
esp_err_t mqtt_transport_stop(void);

#endif /* MQTT_TRANSPORT_H */