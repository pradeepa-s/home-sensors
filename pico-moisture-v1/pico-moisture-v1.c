#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "network.h"
#include "network_conf.h"

extern cyw43_t cyw43_state;

typedef enum{
    WAIT_FOR_SCAN,
    SCAN_COMPLETE,
    CONNECT_TO_WIFI,
    SEND_DATA,
    WAIT_FOR_DATA_COMPLETE,
    WAIT_FOR_TIMEOUT,
    DONE
} STATE;

typedef struct SCAN_RESULT
{
    cyw43_ev_scan_result_t result;
    struct SCAN_RESULT *next;
} SCAN_RESULT_t;


static STATE state = WAIT_FOR_SCAN;
static SCAN_RESULT_t *scan_result = 0;
static const char my_ssid[] = WIFI_SSID;
static const char my_key[] = WIFI_PWORD;
static uint32_t last_send_complete_time = 0;
static uint32_t current_time = 0;

static void print_scan_results();
static void connect_to_wifi();

int wifi_scan_callback(void *arg, const cyw43_ev_scan_result_t* results) {
    if (scan_result == 0) {
        scan_result = calloc(1, sizeof(SCAN_RESULT_t));
        memcpy(&(scan_result->result), results, sizeof(cyw43_ev_scan_result_t));
        scan_result->next = 0;
    }
    else {
        SCAN_RESULT_t *last = scan_result;

        while (last->next) {
            last = last->next;
        }

        SCAN_RESULT_t *new_result = calloc(1, sizeof(SCAN_RESULT_t));
        memcpy(&(new_result->result), results, sizeof(cyw43_ev_scan_result_t));
        last->next = new_result;
        new_result->next = 0;
    }
}

int main()
{
    stdio_init_all();

    if (cyw43_arch_init_with_country(CYW43_COUNTRY_AUSTRALIA) != 0) {
        printf("Failed to initialize \n\r");
        return -1;
    }

    cyw43_arch_enable_sta_mode();

    cyw43_wifi_scan_options_t scan_opt;
    scan_opt.ssid_len = 0;
    scan_opt.scan_type = 0;
    if (cyw43_wifi_scan(&cyw43_state, &scan_opt, NULL, &wifi_scan_callback) != 0) {
        printf("Scan failed to start \n\r");
        return -1;
    }

    while (true) {
        switch (state){
            case WAIT_FOR_SCAN:
                if (!cyw43_wifi_scan_active(&cyw43_state)) {
                    state = SCAN_COMPLETE;
                }
            break;

            case SCAN_COMPLETE:
                printf("Scan completed.\n\r");
                print_scan_results();
                state = CONNECT_TO_WIFI;
            break;

            case CONNECT_TO_WIFI:
                connect_to_wifi();
            break;

            case SEND_DATA:
                start_network();
                state = WAIT_FOR_DATA_COMPLETE;
            break;

            case WAIT_FOR_DATA_COMPLETE:
                if (run_network() == false) {
                    last_send_complete_time = to_ms_since_boot(get_absolute_time());
                    state = WAIT_FOR_TIMEOUT;
                }
            break;

            case WAIT_FOR_TIMEOUT:
                    current_time = to_ms_since_boot(get_absolute_time());
                    if (current_time - last_send_complete_time > 3000) {
                        state = SEND_DATA;
                    }
            break;
            case DONE:
            break;
        }

        run_network();
        cyw43_arch_poll();
    }
}

void print_scan_results()
{
    int i = 0;
    for (SCAN_RESULT_t *result = scan_result; result; result = result->next, i++) {
        printf("\t Result[%d]:\n\r", i);
        printf("\t\t SSID: %-32s\n\r", result->result.ssid);
        printf("\t\t RSSI: %4d\n\r", result->result.rssi);
        printf("\t\t AUTH: %d\n\r", result->result.auth_mode);
        printf("\t-------------------\n\r");
    }
}

void connect_to_wifi()
{
    if (cyw43_arch_wifi_connect_timeout_ms(my_ssid, my_key, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        printf("Failed to connect... retrying...\n\r");
        state = CONNECT_TO_WIFI;
    }
    else {
        const uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
        state = SEND_DATA;
    }
}
