#include "network_controller.h"
#include "network_conf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"


extern cyw43_t cyw43_state;

typedef enum{
    WAITING_FOR_START,
    IDLE,
    CONNECT_TO_SERVER,
    WAITING_FOR_SERVER,
    SEND_DATA,
    READ_RESPONSE,
    FREE_RESOURCES,
    ERROR
} STATE;

typedef struct
{
    ip4_addr_t remote_addr;
    struct tcp_pcb *p_tcp_pcb;
} TCP_CLIENT_T;

TCP_CLIENT_T *client_state = 0;
const char server_ip[] = SERVER_IP;
const uint32_t server_port = SERVER_PORT;
const uint32_t poll_interval_s = 2;
static STATE state = WAITING_FOR_START;
static uint8_t retry_count = 0;

static uint8_t submit_count = 0;
static uint8_t transmitted_count = 0;
static uint32_t queued_data = 0;

static err_t tcp_client_poll();
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_client_err(void *arg, err_t err);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

static void connect_to_server();
static void send_data();

void network_controller_start()
{
    if (client_state == 0) {
        client_state = calloc(1, sizeof(TCP_CLIENT_T));

        if (!client_state) {
            printf("Failed to allocated the client state\n\r");
            return;
        }
    }

    state = IDLE;
}

NETWORK_CONTROLLER_STATUS network_controller_service()
{
    switch (state){
        case CONNECT_TO_SERVER:
            connect_to_server();
        break;

        case SEND_DATA:
            send_data();
        break; 

        case FREE_RESOURCES:
            if (client_state)
            {
                free(client_state);
                client_state = 0;
            }
            state = WAITING_FOR_START;
        break;

        case ERROR:
            if (client_state)
            {
                free(client_state);
                client_state = 0;
            }
        break;

        case READ_RESPONSE:
        case WAITING_FOR_START:
        case WAITING_FOR_SERVER:
        break;

        case IDLE:
            if (submit_count != transmitted_count) {
                network_controller_trigger();
                transmitted_count++;
            }
        break;
    }

    return network_controller_get_status();
}

NETWORK_CONTROLLER_STATUS network_controller_get_status()
{
    if (state == WAITING_FOR_START) {
        return NETWORK_CONTROLLER_STATUS_NOT_STARTED;
    }
    else if (state == IDLE) {
        return NETWORK_CONTROLLER_STATUS_IDLE;
    }
    else if (state == ERROR) {
        return NETWORK_CONTROLLER_STATUS_ERROR;
    }

    return NETWORK_CONTROLLER_STATUS_BUSY;
}

void network_controller_submit_data(uint32_t data)
{
    // TODO: Improve
    submit_count++;
    queued_data = data;
}

void network_controller_trigger()
{
    if (state == NETWORK_CONTROLLER_STATUS_IDLE) {
        state = CONNECT_TO_SERVER;
    }
}

void connect_to_server()
{
    ip4addr_aton(server_ip, &client_state->remote_addr);

    printf("Connecting to %s port %u\n\r", server_ip, server_port);

    client_state->p_tcp_pcb = tcp_new_ip_type(IPADDR_TYPE_V4);

    if (!client_state->p_tcp_pcb) {
        printf("Failed to created the protocol control block\n\r");
        state = FREE_RESOURCES;
        return;
    }

    tcp_poll(client_state->p_tcp_pcb, tcp_client_poll, poll_interval_s * 2);
    tcp_sent(client_state->p_tcp_pcb, tcp_client_sent);
    tcp_recv(client_state->p_tcp_pcb, tcp_client_recv);
    tcp_err(client_state->p_tcp_pcb, tcp_client_err);

    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(client_state->p_tcp_pcb, &client_state->remote_addr, server_port, tcp_client_connected);
    if (err == ERR_OK) {
        retry_count = 0;
        state = WAITING_FOR_SERVER;
    }
    else if (retry_count < 3) {
        retry_count++;
    }
    else {
        retry_count = 0;
        state = ERROR;
    }
    cyw43_arch_lwip_end();
}

void send_data()
{
    // Check whether we can send datas
    const size_t avail_size = tcp_sndbuf(client_state->p_tcp_pcb);

    uint8_t msg[100] = {'*', '*', '*', 'S', 'N', 'S', '0', '0', '1'};
    if (avail_size > sizeof(msg)) {
        memcpy(&msg[9], &queued_data, sizeof(queued_data));
        msg[9 + sizeof(queued_data)] = ':';
        cyw43_arch_lwip_begin();
        if (tcp_write(client_state->p_tcp_pcb, (const void*)msg, sizeof(msg), 0) == ERR_OK) {
            if (tcp_output(client_state->p_tcp_pcb) == ERR_OK) {
                printf ("Submitted: %s \n", msg);
                state = READ_RESPONSE;
            }
            else {
                printf("tcp_output failed\n\r");
                state = ERROR;
            }
        }
        else {
            printf("tcp_write failed\n\r");
            state = ERROR;
        }
        cyw43_arch_lwip_end();
    }
    else {
        printf("Write buffer not enough...\n\r");
        state = ERROR;
    }
}

err_t tcp_client_poll(void *arg, struct tcp_pcb *tpcb)
{
    printf("TCP Poll\n\n");
    return ERR_OK;
} 

err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
    printf("TCP Sent %d bytes\n\r", len);
    return ERR_OK;
}

err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    printf("TCP received, err: %d\n\r", err);

    if (p == NULL) {
        printf("Server closed\n\r");
        tcp_close(tpcb);
        state = IDLE;
    }
    else {
        printf("Receive length: %d\n\r", p->len);
        printf("Received message: %*s\n\r", p->len, p->payload);
        pbuf_free(p);
    }

    return ERR_OK;
}

void tcp_client_err(void *arg, err_t err)
{
    if (err == ERR_RST) {
        printf("Server closed the client connection\n\r");
        state = IDLE;
    }
    else {
        printf("TCP error: %d\n\r", err);
        state = ERROR;
    }
}

err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    // err argument is not used in lwip. It's always ERR_OK
    printf("Connected %d\n\r", err);
    state = SEND_DATA;
    return ERR_OK;
}