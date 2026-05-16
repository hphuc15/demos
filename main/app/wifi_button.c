#include "wifi_button.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_attr.h"

#define WIFI_BTN_GPIO    GPIO_NUM_32
#define WIFI_BTN_HOLD_MS 3000

static esp_timer_handle_t s_btn_timer = NULL;
static wifi_btn_cb_t s_on_hold = NULL;  // callback lên app layer

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

void wifi_button_init(wifi_btn_cb_t on_hold) {
    s_on_hold = on_hold;

    gpio_config_t io = {
        .pin_bit_mask = 1ULL << WIFI_BTN_GPIO,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .intr_type = GPIO_INTR_ANYEDGE,
    };
    gpio_config(&io);

    esp_timer_create_args_t timer_args = { .callback = btn_timer_cb, .name = "btn_timer" };
    esp_timer_create(&timer_args, &s_btn_timer);

    gpio_install_isr_service(0);
    gpio_isr_handler_add(WIFI_BTN_GPIO, btn_isr_handler, NULL);
}