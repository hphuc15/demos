#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

/* ThingsBoard Cloud */
#define TB_HOST         "thingsboard.cloud"
#define TB_DEVICE_TOKEN "algae3"

/* HTTP(S) */
#define HTTP_TELEMETRY_URL ("https://" TB_HOST "/api/v1/" TB_DEVICE_TOKEN "/telemetry")

/* MQTT(S) */
#define MQTT_BROKER_URI "mqtts://" TB_HOST ":8883"
#define MQTT_USERNAME   TB_DEVICE_TOKEN
#define MQTT_PASSWORD   ""
#define MQTT_TELEMETRY_TOPIC "v1/devices/me/telemetry"

#endif /* NETWORK_CONFIG_H */