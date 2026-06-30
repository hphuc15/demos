#include "sensor.h"

#define WIFI_SSID                   "B9 106"
#define WIFI_PASSWORD               "B91062005@"
#define SERVER_URL                  "https://c7.hust-2slab.org/api/v1/BH1750_DEMO/telemetry"
#define SENSOR_READ_INTERVAL_MS     5000

void setup() {
    Serial.begin(115200);
    if (!bh1750_init(WIFI_SSID, WIFI_PASSWORD)) {
        Serial.println("[APP] Sensor init failed.");
        while (true) delay(1000);
    }
}

void loop() {
    bh1750_send(SERVER_URL);
    delay(SENSOR_READ_INTERVAL_MS);
}