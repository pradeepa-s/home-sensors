#ifndef _MEASUREMENT_ENGINE_H
#define _MEASUREMENT_ENGINE_H

#include <stdint.h>

typedef enum {
    MEASUREMENT_ENGINE_STATUS_NOT_STARTED,
    MEASUREMENT_ENGINE_STATUS_INITIALISING,
    MEASUREMENT_ENGINE_STATUS_BUSY,
    MEASUREMENT_ENGINE_STATUS_IDLE,
    MEASUREMENT_ENGINE_STATUS_ERROR
} MEASUREMENT_ENGINE_STATUS;

typedef void (*submit_data_cb_t)(uint32_t reading);

void measurement_engine_start(submit_data_cb_t data_cb);
MEASUREMENT_ENGINE_STATUS measurement_engine_service();
MEASUREMENT_ENGINE_STATUS measurement_engine_get_status();

#endif  // _MEASUREMENT_ENGINE_H