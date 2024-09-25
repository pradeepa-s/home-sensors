#ifndef _WIFI_CONTROLLER_H
#define _WIFI_CONTROLLER_H

typedef enum {
    WIFI_CTRL_STATUS_NOT_STARTED,
    WIFI_CTRL_STATUS_BUSY,
    WIFI_CTRL_STATUS_CONNECTED,
    WIFI_CTRL_STATUS_ERROR
} WIFI_CTRL_STATUS;

void wifi_controller_start();
WIFI_CTRL_STATUS wifi_controller_service();
WIFI_CTRL_STATUS wifi_controller_get_status();

#endif  // _WIFI_CONTROLLER_H