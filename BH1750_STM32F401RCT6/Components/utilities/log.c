#include "log.h"
#include <stdarg.h>
#include <stdio.h>
#include "usart.h"

#define LOG_BUF_SIZE 128U

void log_write(const char *prefix, const char *fmt, ...)
{
    char buf[LOG_BUF_SIZE];
    size_t len = 0;

    if (prefix)
    {
        int n = snprintf(buf, sizeof(buf), "%s", prefix);
        len = (n > 0) ? (size_t)n : 0;
        if (len > sizeof(buf)) len = sizeof(buf);
    }

    va_list args;
    va_start(args, fmt);
    int n = vsnprintf(buf + len, sizeof(buf) - len, fmt, args);
    va_end(args);

    if (n > 0)
    {
        len += (size_t)n;
        if (len > sizeof(buf)) len = sizeof(buf);
    }

    HAL_UART_Transmit(&huart2, (uint8_t *)buf, (uint16_t)len, 100);
}