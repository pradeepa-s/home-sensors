#include "network.h"
#include "network_conf.h"
#include "lwip/tcp.h"
#include "pico/cyw43_arch.h"


extern cyw43_t cyw43_state;

typedef enum{
    IDLE,
    CONNECT_TO_SERVER,
    WAITING_FOR_SERVER,
    SEND_DATA,
    READ_RESPONSE,
    FREE_RESOURCES
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
static STATE state = IDLE;
static uint8_t sequence = 0;

static err_t tcp_client_poll();
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static void tcp_client_err(void *arg, err_t err);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);

static void connect_to_server();
static void send_data();

void start_network()
{
    if (client_state == 0) {
        client_state = calloc(1, sizeof(TCP_CLIENT_T));

        if (!client_state) {
            printf("Failed to allocated the client state\n\r");
            return;
        }

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
    }

    state = CONNECT_TO_SERVER;
}

bool run_network()
{
    switch (state){
        case IDLE:
        break;

        case CONNECT_TO_SERVER:
            connect_to_server();
        break;

        case WAITING_FOR_SERVER:
        break;

        case SEND_DATA:
            send_data();
        break; 

        case READ_RESPONSE:
        break;

        case FREE_RESOURCES:
            free(client_state);
            client_state = 0;
            state = IDLE;
        break;
    }

    return state != IDLE;
}

void connect_to_server()
{
    cyw43_arch_lwip_begin();
    err_t err = tcp_connect(client_state->p_tcp_pcb, &client_state->remote_addr, server_port, tcp_client_connected);
    if (err == ERR_OK) {
        state = WAITING_FOR_SERVER;
    }
    cyw43_arch_lwip_end();
}

void send_data()
{
    // Check whether we can send data
    const size_t avail_size = tcp_sndbuf(client_state->p_tcp_pcb);

    char msg[] = "Sequence number 0\n\r";
    if (avail_size > sizeof(msg)) {
        msg[16] = '0' + (msg[16] - '0') + (sequence % 10);
        sequence = (sequence + 1) % 10;

        cyw43_arch_lwip_begin();
        if (tcp_write(client_state->p_tcp_pcb, (const void*)msg, sizeof(msg), 0) == ERR_OK) {
            if (tcp_output(client_state->p_tcp_pcb) == ERR_OK) {
                printf ("Submitted: %s \n", msg);
                state = READ_RESPONSE;
            }
            else {
                printf("tcp_output failed\n\r");
                state = FREE_RESOURCES;
            }
        }
        else {
            printf("tcp_write failed\n\r");
            state = FREE_RESOURCES;
        }
        cyw43_arch_lwip_end();
    }
    else {
        printf("Write buffer not enough...\n\r");
        state = FREE_RESOURCES;
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
        state = FREE_RESOURCES;
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
    }
    else {
        printf("TCP error: %d\n\r", err);
    }
    state = FREE_RESOURCES;
}

err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    printf("Connected %d\n\r", err);
    state = SEND_DATA;
    return ERR_OK;
}