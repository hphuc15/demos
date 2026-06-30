// hardware_config.h
#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

typedef void (*wifi_btn_cb_t)(void);

typedef enum {
    HW_LED_OFF,
    HW_LED_ON,
    HW_LED_BLINK,
} hw_led_mode_t;

void hw_init(wifi_btn_cb_t on_hold);
void hw_led_set(hw_led_mode_t mode);

#endif