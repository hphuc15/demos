# BH1750_Demo

## Overview

Firmware built with **ESP-IDF**. Collects BH1750 data and publishes to server via HTTPS or MQTTS.

### Behavior

* **Status LED**

  * `ON` → Device connected to Wi-Fi AP
  * `OFF` → Wi-Fi disconnected
  * `BLINK` → Wi-Fi configuration mode active

* **Wi-Fi Config Button**

  * Press and hold for `3 seconds` to enter Wi-Fi configuration mode

### Hardware Connection

### ESP32 - BH1750 (GY-302)

| BH1750 Pin | ESP32 Pin | Description                     |
| ---------- | --------- | ------------------------------- |
| `VCC`      | `3V3`     | Power supply                    |
| `GND`      | `GND`     | Ground                          |
| `SDA`      | `GPIO21`  | I2C data line                   |
| `SCL`      | `GPIO22`  | I2C clock line                  |
| `ADDR`     | `GND`     | I2C address (`0x23` by default) |

### Feature Configuration

| Feature       | GPIO / Function |
| ------------- | --------------- |
| Status LED    | `GPIO2`         |
| Config Button | `GPIO32`        |
| USB-UART      | `CH340C`        |
| USB Connector | `USB Type-C`    |

---

## Setup

### 1. Configure `network_config.h`
Fill in `./components/network/include/network_config.h` with your credentials:
```c
#ifndef NETWORK_CONFIG_H
#define NETWORK_CONFIG_H

#include "esp_err.h"
#include <stddef.h>
#include <stdbool.h>

/* Server Credentials */
#define SERVER_DEFAULT_HOST             "<default_server_host>"
#define SERVER_DEFAULT_PORT             <default_server_port>     /* 443 for MQTTS, 1883 for MQTT, 8883 for MQTTS*/
#define DEVICE_TOKEN_DEFAULT            "<default_device_token>"  /* BH1750_DEMO for Sensor Lab ThingsBoard server in this demo */
/* Network NVS Credentials */
#define NETWORK_CONFIG_NVS_NAMESPACE    "network_nvs"
#define NETWORK_CONFIG_NVS_HOST_KEY     "host"
#define NETWORK_CONFIG_NVS_PORT_KEY     "port"
#define NETWORK_CONFIG_NVS_TOKEN_KEY    "token"
/* ESP32 WiFi AP Credentials */
#define NETWORK_CONFIG_WIFI_AP_SSID     "BH1750_DEMO"
#define NETWORK_CONFIG_WIFI_AP_PASSWORD "BH1750_DEMO"

#endif /* NETWORK_CONFIG_H */
```

### 2. Build anf flash firmware
### 3. Connect to Wi-Fi via captive portal

On first boot (or after holding the config button for 3 seconds), the device starts in AP mode:

* Connect your phone/PC to Wi-Fi: **`BH1750_DEMO`** (password: `BH1750_DEMO`)
* A captive portal opens automatically - fill in:

| Field   | Example                   | Description              |
| ------- | ------------------------- | ------------------------ |
| `Host`  | `thingsboard.cloud`       | Broker or server host    |
| `Port`  | `1883` / `8883` / `443`   | Port determines protocol |
| `Token` | `your_device_token`       | Device access token      |

* Submit and the device connects to your AP. Config is saved to NVS and reused on next boot.

### 4. Protocol selection

Protocol is selected automatically based on port:

| Port   | Protocol |
| ------ | -------- |
| `1883` | MQTT     |
| `8883` | MQTTS    |
| others | HTTPS    |

> **Note:** In this demo, `device_token` was set to `BH1750_DEMO`, used for the Sensor Lab ThingsBoard server.

---

## Project Structure

```text
📁 components
├── hardware                        # Config button, status LED
│   ├── hardware_config.h
│   ├── hardware_config.c
│   └── CMakeLists.txt
│
├── network                         # Network layer
│   ├── include
│   │   ├── network.h
│   │   └── network_config.h        # Network credentials
│   ├── transport
│   │   ├── http                    # HTTPS transport API
│   │   │   ├── http_transport.h
│   │   │   └── http_transport.c
│   │   └── mqtt                    # MQTTS transport API
│   │       ├── mqtt_transport.h
│   │       └── mqtt_transport.c
│   ├── wifi
│   │   ├── WiFiManager             # hphuc15/WiFiManager
│   │   ├── wifi.c
│   │   └── wifi.h
│   ├── CMakeLists.txt
│   └── network.c
│
└── sensors                         # Sensor acquisition
    ├── bare-drivers                # hphuc15/bare-drivers
    │   └── bh1750
    │       ├── include
    │       │   └── bh1750.h
    │       └── bh1750.c
    ├── i2c                         # I2C bus and device configuration
    │   ├── i2c.h
    │   └── i2c.c
    ├── sensors.h                   # Sensors layer
    ├── sensors.c
    └── CMakeLists.txt

📁 main
├── app                             # Application layer
│   ├── app.h
│   └── app.c
├── main.c
└── CMakeLists.txt

📁 docs                            # Documentation
├── image
│   └── FLOW.png
└── tree.py
```

---

## Flow

<p align="center">
  <img src="./docs/image/FLOW.png" alt="Flow" width="700"/>
</p>