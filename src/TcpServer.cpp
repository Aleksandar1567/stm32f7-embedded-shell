// src/TcpServer.cpp — TCP echo server on port 7, lwIP netconn API

#include "TcpServer.hpp"
#include "Uart.hpp"
#include "lwip/api.h"
#include "FreeRTOS.h"
#include "task.h"

#define TCP_SERVER_PORT  7
#define RX_BUF_SIZE      512

extern Uart dbg;

// ── Per-connection handler ─────────────────────────────────────────────────
static void handle_connection(struct netconn *conn)
{
    struct netbuf *buf;
    void    *data;
    u16_t    len;
    err_t    err;

    // Get remote address for logging
    ip_addr_t remote_ip;
    u16_t     remote_port;
    netconn_peer(conn, &remote_ip, &remote_port);
    char ip_str[16];
    ipaddr_ntoa_r(&remote_ip, ip_str, sizeof(ip_str));
    dbg.printf("[tcp] client connected: %s:%u\r\n", ip_str, remote_port);

    // Echo loop
    while ((err = netconn_recv(conn, &buf)) == ERR_OK) {
        do {
            netbuf_data(buf, &data, &len);
            dbg.printf("[tcp] rx %u bytes\r\n", len);
            err = netconn_write(conn, data, len, NETCONN_COPY);
            if (err != ERR_OK) {
                dbg.printf("[tcp] tx error: %d\r\n", (int)err);
                break;
            }
        } while (netbuf_next(buf) >= 0);
        netbuf_delete(buf);

        if (err != ERR_OK) break;
    }

    dbg.printf("[tcp] client disconnected: %s:%u (err=%d)\r\n",
               ip_str, remote_port, (int)err);
    netconn_close(conn);
    netconn_delete(conn);
}

// ── Server task ────────────────────────────────────────────────────────────
static void tcp_server_task(void *)
{
    struct netconn *listen_conn;
    struct netconn *client_conn;
    err_t err;

    // Wait until network is up
    vTaskDelay(pdMS_TO_TICKS(2000));

    listen_conn = netconn_new(NETCONN_TCP);
    if (listen_conn == NULL) {
        dbg.println("[tcp] netconn_new failed");
        vTaskDelete(NULL);
        return;
    }

    err = netconn_bind(listen_conn, IP_ADDR_ANY, TCP_SERVER_PORT);
    if (err != ERR_OK) {
        dbg.printf("[tcp] bind failed: %d\r\n", (int)err);
        netconn_delete(listen_conn);
        vTaskDelete(NULL);
        return;
    }

    netconn_listen(listen_conn);
    dbg.printf("[tcp] echo server listening on port %d\r\n", TCP_SERVER_PORT);

    for (;;) {
        err = netconn_accept(listen_conn, &client_conn);
        if (err == ERR_OK) {
            handle_connection(client_conn);
        } else {
            dbg.printf("[tcp] accept error: %d\r\n", (int)err);
            vTaskDelay(pdMS_TO_TICKS(100));
        }
    }
}

// ── Public API ─────────────────────────────────────────────────────────────
void TcpServer_Start(void)
{
    xTaskCreate(tcp_server_task, "tcp_srv", 512, NULL, 4, NULL);
}
