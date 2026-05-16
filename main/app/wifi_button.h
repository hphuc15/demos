#ifndef WIFI_BUTTON_H
#define WIFI_BUTTON_H

typedef void (*wifi_btn_cb_t)(void);
void wifi_button_init(wifi_btn_cb_t on_hold);

#endif /* WIFI_BUTTON_H */