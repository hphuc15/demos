#ifndef WIFI_H
#define WIFI_H

#include <stdbool.h>

typedef void (*wifi_cb_t)(void);

/* Initialize WiFi */
void wifi_init(void);
/* Start WiFi */
void wifi_connect(void);
/* Stop WiFi */
void wifi_stop(void);

/* Set extenal WiFi connected callback */
void wifi_set_connected_cb(wifi_cb_t cb);
/* Set extenal WiFi disconnected callback */
void wifi_set_disconnected_cb(wifi_cb_t cb);

bool wifi_is_ready(void);

#endif /* WIFI_H */