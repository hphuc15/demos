## Setup
Fill in `sensor.h`:
```c
#pragma once

#include <stdbool.h>

/* Sensor pin */
#define BH1750_I2C_SDA_IO           21          /* Chân SDA của bh1750 */
#define BH1750_I2C_SCL_IO           22          /* Chân SCL của bh1750 */
/* WiFi timeout */
#define WIFI_CONNECT_TIMEOUT_MS     15000       /* -1 = vô hạn (vô hạn thì block cảm biến) */
/* HTTP */
#define HTTP_POST_KEY               "light"     /* data json key */
#define HTTP_POST_TIMEOUT_MS        5000


/**
 * @brief Khởi tạo bh1750 và WiFi.
 * @param ssid      Tên WiFi
 * @param password  Mật khẩu WiFi
 * @return true nếu ok, false nếu lỗi
 */
bool bh1750_init(const char *ssid, const char *password);

/**
 * @brief Đọc cường độ ánh sáng từ bh1750 và gửi lên server.
 * @return true nếu response 200/201
 */
bool bh1750_send(const char *server_url);
```