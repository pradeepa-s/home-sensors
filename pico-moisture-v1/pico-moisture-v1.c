#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "network_controller.h"
#include "wifi_controller.h"
#include "measurement_engine.h"

extern cyw43_t cyw43_state;

typedef enum{
    START_WIFI_CONTROLLER,
    SERVICE_WIFI_CONTROLLER,
    START_CORE,
    SERVICE_CORE,
    APP_RECOVERY
} STATE;

typedef enum{
    TD_SEND,
    TD_WAIT_SEND_COMPLETE,
    TD_WAIT_FOR_RETRIGGER
} TD_STATE;

static TD_STATE test_data_state = TD_SEND;
static STATE state = START_WIFI_CONTROLLER;
static uint32_t last_send_complete_time = 0;
static void test_data_trigger();


void measurement_data_available(uint32_t data) {
    network_controller_submit_data(data);
}

int main()
{
    stdio_init_all();

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRALIA) != 0) {
        printf("Failed to initialize \n\r");
        return -1;
    }

    while (true) {
        switch (state)
        {
        case START_WIFI_CONTROLLER:
            wifi_controller_start();
            state = SERVICE_WIFI_CONTROLLER;
            break;

        case SERVICE_WIFI_CONTROLLER:
            wifi_controller_service();

            if (wifi_controller_get_status() == WIFI_CTRL_STATUS_CONNECTED) {
                state = START_CORE;
            }
            else if (wifi_controller_get_status() == WIFI_CTRL_STATUS_ERROR) {
                state = APP_RECOVERY;
            }
            break;

        case START_CORE:
            network_controller_start();
            measurement_engine_start(measurement_data_available);
            state = SERVICE_CORE;
            break;

        case SERVICE_CORE:
            // test_data_trigger();
            measurement_engine_service();
            network_controller_service();

            if (measurement_engine_get_status() == MEASUREMENT_ENGINE_STATUS_ERROR) {
                state = APP_RECOVERY;
            }

            if (network_controller_get_status() == NETWORK_CONTROLLER_STATUS_ERROR) {
                state = APP_RECOVERY;
            }

            break;

        case APP_RECOVERY:
            break;

        default:
            break;
        }

        cyw43_arch_poll();
    }
}

void test_data_trigger()
{
    uint32_t current_time = 0;

    switch (test_data_state){
    case TD_SEND:
        if (network_controller_get_status() == NETWORK_CONTROLLER_STATUS_IDLE) {
            // network_controller_trigger();
            test_data_state = TD_WAIT_SEND_COMPLETE;
        }
    break;

    case TD_WAIT_SEND_COMPLETE:
        if (network_controller_get_status() == NETWORK_CONTROLLER_STATUS_IDLE) {
            test_data_state = TD_WAIT_FOR_RETRIGGER;
            last_send_complete_time = to_ms_since_boot(get_absolute_time());
        }
    break;

    case TD_WAIT_FOR_RETRIGGER:
        current_time = to_ms_since_boot(get_absolute_time());
        if (current_time - last_send_complete_time > 3000) {
            test_data_state = TD_SEND;
        }
    break;

    default:
    break;
    }
}
