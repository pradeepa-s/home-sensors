#include "wifi_controller.h"
#include "pico/cyw43_arch.h"
#include "network_conf.h"


typedef struct SCAN_RESULT
{
    cyw43_ev_scan_result_t result;
    struct SCAN_RESULT *next;
} SCAN_RESULT_t;

typedef enum{
    IDLE,
    START_SCAN,
    WAIT_FOR_SCAN,
    SCAN_COMPLETE,
    CONNECT_TO_WIFI,
    CONNECTED,
    ERROR
} STATE;

static STATE state = IDLE;
static SCAN_RESULT_t *scan_result = 0;
static const char my_ssid[] = WIFI_SSID;
static const char my_key[] = WIFI_PWORD;
static cyw43_wifi_scan_options_t scan_opt;
static uint8_t retry_count = 0;

static int wifi_scan_callback(void *arg, const cyw43_ev_scan_result_t* results);
static void print_scan_results();
static void connect_to_wifi();

void wifi_controller_start()
{
    // TODO: Free results
    cyw43_arch_enable_sta_mode();

    state = START_SCAN;
}

WIFI_CTRL_STATUS wifi_controller_service()
{
    switch (state)
    {
        case START_SCAN:
            scan_opt.ssid_len = 0;
            scan_opt.scan_type = 0;
            if (cyw43_wifi_scan(&cyw43_state, &scan_opt, NULL, &wifi_scan_callback) != 0) {
                printf("Scan failed to start \n\r");
                state = ERROR;
            }
            else {
                state = WAIT_FOR_SCAN;
            }
        break;

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

        case CONNECTED:
        case IDLE:
        case ERROR:
        default:
        break;
    }

    return wifi_controller_get_status();
}

WIFI_CTRL_STATUS wifi_controller_get_status()
{
    if (state == CONNECTED) {
        return WIFI_CTRL_STATUS_CONNECTED;
    }
    else if (state == ERROR) {
        return WIFI_CTRL_STATUS_ERROR;
    }
    else if (state == IDLE) {
        return WIFI_CTRL_STATUS_NOT_STARTED;
    }

    return WIFI_CTRL_STATUS_BUSY;
}

void connect_to_wifi()
{
    if (cyw43_arch_wifi_connect_timeout_ms(my_ssid, my_key, CYW43_AUTH_WPA2_AES_PSK, 30000) != 0) {
        retry_count++;
        if (retry_count > 3) {
            printf("Failed to connect...\n\r");
            state = ERROR;
        }
        else {
            printf("Failed to connect... retrying... %d\n\r", retry_count);
            state = CONNECT_TO_WIFI;
        }
    }
    else {
        retry_count = 0;
        const uint8_t *ip_address = (uint8_t*)&(cyw43_state.netif[0].ip_addr.addr);
        printf("IP address %d.%d.%d.%d\n", ip_address[0], ip_address[1], ip_address[2], ip_address[3]);
        state = CONNECTED;
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
