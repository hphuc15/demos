// hardware_config.c
#include "hardware_config.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"

#define STATUS_LED_GPIO  GPIO_NUM_2
#define WIFI_BTN_GPIO    GPIO_NUM_32
#define WIFI_BTN_HOLD_MS 3000
#define LED_BLINK_MS     500

/* ------------------------------------------------------------------
 *  Status LED
 * ------------------------------------------------------------------ */

static esp_timer_handle_t s_led_timer = NULL;
static hw_led_mode_t      s_led_mode  = HW_LED_OFF;
static int s_led_state = 0;

static void led_blink_cb(void *arg) {
    s_led_state ^= 1;
    gpio_set_level(STATUS_LED_GPIO, s_led_state);
}

static void led_init(void) {
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << STATUS_LED_GPIO,
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    gpio_config(&cfg);
    gpio_set_level(STATUS_LED_GPIO, 0);

    esp_timer_create_args_t args = {
        .callback = led_blink_cb,
        .name     = "led_blink",
    };
    esp_timer_create(&args, &s_led_timer);
}

void hw_led_set(hw_led_mode_t mode) {
    if (mode == s_led_mode){
        return;
    }
    s_led_mode = mode;
    esp_timer_stop(s_led_timer);

    if (s_led_timer){
        esp_timer_stop(s_led_timer);
    }

    switch (mode) {
        case HW_LED_OFF:
            s_led_state = 0;
            gpio_set_level(STATUS_LED_GPIO, 0);
            break;
        case HW_LED_ON:
            s_led_state = 1;
            gpio_set_level(STATUS_LED_GPIO, 1);
            break;
        case HW_LED_BLINK:
            s_led_state = 1;
            gpio_set_level(STATUS_LED_GPIO, 1);
            esp_timer_start_periodic(s_led_timer, LED_BLINK_MS * 1000ULL);
            break;
    }
}

/* ------------------------------------------------------------------
 *  Config button
 * ------------------------------------------------------------------ */

static esp_timer_handle_t s_btn_timer = NULL;
static wifi_btn_cb_t      s_on_hold   = NULL;

static void btn_timer_cb(void *arg) {
    if (gpio_get_level(WIFI_BTN_GPIO) == 0 && s_on_hold) {
        s_on_hold();
    }
}

static void IRAM_ATTR btn_isr_handler(void *arg) {
    if (gpio_get_level(WIFI_BTN_GPIO) == 0) {
        esp_timer_start_once(s_btn_timer, WIFI_BTN_HOLD_MS * 1000ULL);
    } else {
        esp_timer_stop(s_btn_timer);
    }
}

static void button_init(wifi_btn_cb_t on_hold) {
    s_on_hold = on_hold;

    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << WIFI_BTN_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&cfg);

    esp_timer_create_args_t args = {
        .callback = btn_timer_cb,
        .name     = "btn_timer",
    };
    esp_timer_create(&args, &s_btn_timer);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIFI_BTN_GPIO, btn_isr_handler, NULL);
}

/* ------------------------------------------------------------------
 *  Public API
 * ------------------------------------------------------------------ */

void hw_init(wifi_btn_cb_t on_hold) {
    led_init();
    button_init(on_hold);
}