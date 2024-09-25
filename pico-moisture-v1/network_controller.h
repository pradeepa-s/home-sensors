#ifndef _NETWORK_CONTROLLER_H
#define _NETWORK_CONTROLLER_H

#include <stdint.h>

typedef enum {
    NETWORK_CONTROLLER_STATUS_NOT_STARTED,
    NETWORK_CONTROLLER_STATUS_IDLE,
    NETWORK_CONTROLLER_STATUS_BUSY,
    NETWORK_CONTROLLER_STATUS_ERROR
} NETWORK_CONTROLLER_STATUS;

void network_controller_start();
NETWORK_CONTROLLER_STATUS network_controller_service();
NETWORK_CONTROLLER_STATUS network_controller_get_status();
void network_controller_submit_data(uint32_t data);
void network_controller_trigger();

#endif  // _NETWORK_CONTROLLER_H