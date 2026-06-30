#include "sensor.h"

#include <Arduino.h>
#include <Wire.h>
#include <BH1750.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ArduinoJson.h>

static BH1750 s_light_meter;

static void wifi_connect(const char *ssid, const char *password) {
    Serial.printf("[WiFi] Connecting to %s", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    uint32_t start = millis();
    while (WiFi.status() != WL_CONNECTED) {
        if (WIFI_CONNECT_TIMEOUT_MS >= 0 &&
            millis() - start > (uint32_t)WIFI_CONNECT_TIMEOUT_MS) {
            Serial.println("\n[WiFi] Timeout!");
            return;
        }
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
}

/* Publics API */

bool bh1750_init(const char *ssid, const char *password) {
    Wire.begin(BH1750_I2C_SDA_IO, BH1750_I2C_SCL_IO);

    if (!s_light_meter.begin(BH1750::CONTINUOUS_HIGH_RES_MODE)) {
        Serial.println("[BH1750] Init FAILED!");
        return false;
    }
    Serial.println("[BH1750] Ready.");

    WiFi.setAutoReconnect(true);
    wifi_connect(ssid, password);
    return true;
}

bool bh1750_send(const char *server_url) {
    float lux = s_light_meter.readLightLevel();
    if (lux < 0) {
        Serial.println("[BH1750] Read error");
        return false;
    }
    Serial.printf("[BH1750] %.2f lux\n", lux);

    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("[HTTP] Skip, no WiFi.");
        return false;
    }

    StaticJsonDocument<64> doc;
    doc[HTTP_POST_KEY] = lux;
    String payload;
    serializeJson(doc, payload);

    HTTPClient http;
    http.begin(server_url);
    http.setTimeout(HTTP_POST_TIMEOUT_MS);
    http.addHeader("Content-Type", "application/json");

    int code = http.POST(payload);
    bool ok  = (code == HTTP_CODE_OK || code == HTTP_CODE_CREATED);

    if (code > 0) {
        Serial.printf("[HTTP] %d %s\n", code, ok ? "OK" : http.getString().c_str());
    } else {
        Serial.printf("[HTTP] Error: %s\n", HTTPClient::errorToString(code).c_str());
    }

    http.end();
    return ok;
}