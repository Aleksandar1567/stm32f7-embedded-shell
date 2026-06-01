// src/Network.cpp — lwIP stack init + DHCP + netif management

#include "Network.hpp"
#include "EthernetIF.hpp"
#include "Uart.hpp"

#include <stdarg.h>
#include <stdio.h>

#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/dhcp.h"
#include "lwip/ip_addr.h"
#include "ethernetif.h"

#include "FreeRTOS.h"
#include "task.h"

extern Uart dbg;

extern uint32_t ethernetif_rx_count;
extern uint32_t ethernetif_rx_drop;

/* C-callable log function used by ethernetif.c */
extern "C" void eth_log(const char *fmt, ...)
{
    char buf[128];
    va_list args;
    va_start(args, fmt);
    vsnprintf(buf, sizeof(buf), fmt, args);
    va_end(args);
    dbg.print(buf);
}

static struct netif s_netif;
static volatile bool s_dhcp_up = false;

// ── DHCP polling task ─────────────────────────────────────────────────────
static void dhcpTask(void *)
{
    uint32_t tries = 0;

    // Wait for Ethernet link
    while (!ETH_IsLinkUp()) {
        vTaskDelay(pdMS_TO_TICKS(200));
    }

    netif_set_link_up(&s_netif);
    netif_set_up(&s_netif);

    dhcp_start(&s_netif);
    dbg.println("[net] DHCP started, waiting for IP...");

    for (;;) {
        if (dhcp_supplied_address(&s_netif)) {
            if (!s_dhcp_up) {
                s_dhcp_up = true;
                Network_PrintInfo();
            }
        } else {
            s_dhcp_up = false;
            tries++;
            if (tries > 50) {        /* ~10 s — fallback static IP */
                tries = 0;
                dhcp_stop(&s_netif);
                ip4_addr_t ip, nm, gw;
                IP4_ADDR(&ip, 192, 168, 1, 200);
                IP4_ADDR(&nm, 255, 255, 255, 0);
                IP4_ADDR(&gw, 192, 168, 1, 1);
                netif_set_addr(&s_netif, &ip, &nm, &gw);
                s_dhcp_up = true;
                dbg.println("[net] DHCP timeout — using static 192.168.1.200");
                Network_PrintInfo();
            }
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}

// ── Ethernet polling task ─────────────────────────────────────────────────
static void ethPollTask(void *)
{
    uint32_t log_tick = 0;
    for (;;) {
        ethernetif_poll(&s_netif);

        /* Log every 2 s: link state + rx packet count */
        if (++log_tick >= 400) {
            log_tick = 0;
            bool link = ETH_IsLinkUp();
            dbg.printf("[eth] link=%s  rx=%lu  drop=%lu\r\n",
                       link ? "UP" : "DOWN",
                       ethernetif_rx_count,
                       ethernetif_rx_drop);
        }

        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// ── Public API ────────────────────────────────────────────────────────────
void Network_Init(void)
{
    // Init lwIP core (starts tcpip thread internally)
    tcpip_init(NULL, NULL);

    // Add network interface
    ip4_addr_t ip = {0}, nm = {0}, gw = {0};  // zeroes = use DHCP
    netif_add(&s_netif, &ip, &nm, &gw, NULL, ethernetif_init, tcpip_input);
    netif_set_default(&s_netif);
    netif_set_hostname(&s_netif, "stm32-f767");

    // Polling tasks
    xTaskCreate(ethPollTask, "eth_poll", 256, NULL, 6, NULL);
    xTaskCreate(dhcpTask,    "dhcp",     256, NULL, 4, NULL);

    dbg.println("[net] lwIP init OK");
}

bool Network_IsUp(void) { return s_dhcp_up; }

void Network_PrintInfo(void)
{
    char ip[16], nm[16], gw[16];
    ip4addr_ntoa_r(netif_ip4_addr(&s_netif),    ip, sizeof(ip));
    ip4addr_ntoa_r(netif_ip4_netmask(&s_netif), nm, sizeof(nm));
    ip4addr_ntoa_r(netif_ip4_gw(&s_netif),      gw, sizeof(gw));
    dbg.printf("[net] IP=%s  NM=%s  GW=%s\r\n", ip, nm, gw);
}
