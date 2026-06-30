#pragma once

#include <stdbool.h>

/* Sensor pin */
#define BH1750_I2C_SDA_IO           21
#define BH1750_I2C_SCL_IO           22
/* WiFi timeout */
#define WIFI_CONNECT_TIMEOUT_MS     15000      /* -1 = vô hạn */
/* HTTP */
#define HTTP_POST_KEY               "light"
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