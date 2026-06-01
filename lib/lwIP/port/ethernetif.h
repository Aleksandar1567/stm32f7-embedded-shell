/* port/ethernetif.h — lwIP netif driver for STM32F7 HAL ETH */
#pragma once

#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Call once from the network init task.
   Adds and brings up the netif; starts DHCP if dhcp=true. */
err_t ethernetif_init(struct netif *netif);

/* Called from the ETH IRQ / receive task to pass a frame to lwIP */
void ethernetif_input(struct netif *netif);

/* Call periodically (e.g. every 100 ms) to check for new Rx frames (polling). */
void ethernetif_poll(struct netif *netif);

#ifdef __cplusplus
}
#endif
