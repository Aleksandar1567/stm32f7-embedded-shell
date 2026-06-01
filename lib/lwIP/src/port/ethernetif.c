/* port/ethernetif.c — lwIP netif driver for STM32F767ZI HAL ETH */

#include "lwip/opt.h"
#include "lwip/netif.h"
#include "lwip/etharp.h"
#include "lwip/pbuf.h"
#include "lwip/err.h"
#include "ethernetif.h"
#include "stm32f7xx_hal.h"
#include <string.h>
#include <stdio.h>

/* Forward declare heth from EthernetIF.cpp */
extern ETH_HandleTypeDef heth;

/* C-callable logger provided by Network.cpp */
extern void eth_log(const char *fmt, ...);

/* Diagnostics counters — read from Network.cpp */
uint32_t ethernetif_rx_count = 0;
uint32_t ethernetif_rx_drop  = 0;

/* ── Rx buffer pool ─────────────────────────────────────────────────────── */
/* HAL needs pre-allocated Rx buffers for DMA descriptors. */

#define RX_POOL_SIZE   ETH_RX_DESC_CNT

static uint8_t RxBuf[RX_POOL_SIZE][ETH_MAX_PACKET_SIZE] __attribute__((aligned(4)));
static uint8_t RxBufInUse[RX_POOL_SIZE];

/* Give HAL a free Rx buffer for a DMA descriptor */
void HAL_ETH_RxAllocateCallback(uint8_t **buff)
{
    for (int i = 0; i < RX_POOL_SIZE; i++) {
        if (!RxBufInUse[i]) {
            RxBufInUse[i] = 1;
            *buff = RxBuf[i];
            return;
        }
    }
    eth_log("[eth] RxAllocate: OUT OF BUFFERS\r\n");
    *buff = NULL;
}

/* Called per DMA descriptor fragment — copy into a PBUF_POOL pbuf so the
   DMA buffer can be returned to the pool immediately (safe for async tcpip). */
void HAL_ETH_RxLinkCallback(void **pStart, void **pEnd,
                             uint8_t *buff, uint16_t length)
{
    /* Decode EtherType from first fragment (bytes 12-13 of Ethernet frame) */
    if (*pStart == NULL && length >= 14) {
        uint16_t etype = (uint16_t)((buff[12] << 8) | buff[13]);
        const char *name = "???";
        if      (etype == 0x0800) name = "IPv4";
        else if (etype == 0x0806) name = "ARP";
        else if (etype == 0x86DD) name = "IPv6";
        eth_log("[eth] RX frame len=%u type=0x%04X(%s)\r\n", length, etype, name);
    }

    /* Copy data out of the DMA buffer into an lwIP pool pbuf */
    struct pbuf *p = pbuf_alloc(PBUF_RAW, length, PBUF_POOL);

    /* Return the DMA buffer to pool regardless of pbuf allocation */
    for (int i = 0; i < RX_POOL_SIZE; i++) {
        if (buff == RxBuf[i]) { RxBufInUse[i] = 0; break; }
    }

    if (p == NULL) {
        eth_log("[eth] RxLink: pbuf_alloc FAILED len=%u\r\n", length);
        return;
    }

    pbuf_take(p, buff, length);
    p->next = NULL;

    if (*pStart == NULL) {
        *pStart = p;
        *pEnd   = p;
    } else {
        pbuf_chain((struct pbuf *)*pStart, p);
        *pEnd = p;
    }
}

/* ── Free Tx pbufs after transmission ───────────────────────────────────── */
/* Not used with blocking Transmit — provided for completeness. */
void HAL_ETH_TxFreeCallback(uint32_t *buff)
{
    pbuf_free((struct pbuf *)buff);
}

/* ── low_level_output — called by lwIP to send a frame ─────────────────── */
static err_t low_level_output(struct netif *netif, struct pbuf *p)
{
    (void)netif;
    uint32_t i = 0;
    struct pbuf *q;
    ETH_TxPacketConfigTypeDef txcfg = {0};
    ETH_BufferTypeDef txbuf[5] = {0};

    /* Log what we're about to send */
    if (p->tot_len >= 14) {
        uint8_t *hdr = (uint8_t *)p->payload;
        uint16_t etype = (uint16_t)((hdr[12] << 8) | hdr[13]);
        const char *name = "???";
        if      (etype == 0x0800) name = "IPv4";
        else if (etype == 0x0806) name = "ARP";
        else if (etype == 0x86DD) name = "IPv6";
        eth_log("[eth] TX frame len=%u type=0x%04X(%s)\r\n",
                (unsigned)p->tot_len, etype, name);
    }

    for (q = p; q != NULL && i < 5; q = q->next, i++) {
        txbuf[i].buffer = (uint8_t *)q->payload;
        txbuf[i].len    = q->len;
        txbuf[i].next   = (q->next && (i + 1) < 5) ? &txbuf[i + 1] : NULL;
    }

    txcfg.Length   = p->tot_len;
    txcfg.TxBuffer = txbuf;
    txcfg.pData    = (uint32_t *)p;

    HAL_StatusTypeDef hal_res = HAL_ETH_Transmit(&heth, &txcfg, 100);
    if (hal_res != HAL_OK) {
        eth_log("[eth] TX HAL_ETH_Transmit FAILED err=0x%lx\r\n", heth.ErrorCode);
    } else {
        eth_log("[eth] TX OK\r\n");
    }

    /* Release Tx descriptor slot so it can be reused for the next packet. */
    HAL_ETH_ReleaseTxPacket(&heth);

    return (hal_res == HAL_OK) ? ERR_OK : ERR_IF;
}

/* ── ethernetif_init — netif init function ──────────────────────────────── */
err_t ethernetif_init(struct netif *netif)
{
    netif->name[0] = 's';
    netif->name[1] = 't';
    netif->output     = etharp_output;
    netif->linkoutput = low_level_output;
    netif->mtu        = 1500;
    netif->flags      = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP;

    netif->hwaddr_len = ETH_HWADDR_LEN;
    for (int i = 0; i < ETH_HWADDR_LEN; i++) {
        netif->hwaddr[i] = heth.Init.MACAddr[i];
    }

    eth_log("[eth] netif init: MAC=%02X:%02X:%02X:%02X:%02X:%02X\r\n",
            netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2],
            netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]);
    return ERR_OK;
}

/* ── ethernetif_poll — call from a task to process received frames ───────── */
void ethernetif_poll(struct netif *netif)
{
    struct pbuf *p = NULL;

    while (HAL_ETH_ReadData(&heth, (void **)&p) == HAL_OK) {
        if (p == NULL) break;
        ethernetif_rx_count++;
        err_t err = netif->input(p, netif);
        if (err != ERR_OK) {
            eth_log("[eth] netif->input ERR=%d\r\n", (int)err);
            pbuf_free(p);
            ethernetif_rx_drop++;
        }
        p = NULL;
    }
}
