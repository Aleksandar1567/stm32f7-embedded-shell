/* port/lwipopts.h — lwIP configuration for STM32F767ZI + FreeRTOS */
#pragma once

/* ── OS mode ─────────────────────────────────────────────────────────────── */
#define NO_SYS                      0   /* Use FreeRTOS */
#define LWIP_NETCONN                1
#define LWIP_SOCKET                 0   /* Disable BSD sockets to save RAM */

/* ── Memory ──────────────────────────────────────────────────────────────── */
#define MEM_ALIGNMENT               4
#define MEM_SIZE                    (16 * 1024)  /* Heap for lwIP */
#define MEMP_NUM_PBUF               16
#define MEMP_NUM_TCP_SEG            24
#define MEMP_NUM_TCP_PCB            8
#define MEMP_NUM_TCP_PCB_LISTEN     4
#define MEMP_NUM_UDP_PCB            4
#define MEMP_NUM_NETBUF             8
#define MEMP_NUM_NETCONN            8
#define MEMP_NUM_TCPIP_MSG_API      16
#define MEMP_NUM_TCPIP_MSG_INPKT    16

/* ── Buffers ─────────────────────────────────────────────────────────────── */
#define PBUF_POOL_SIZE              8
#define PBUF_POOL_BUFSIZE           1528

/* ── Protocols ───────────────────────────────────────────────────────────── */
#define LWIP_ARP                    1
#define LWIP_ETHERNET               1
#define LWIP_IPV4                   1
#define LWIP_ICMP                   1
#define LWIP_RAW                    1
#define LWIP_DHCP                   1
#define LWIP_TCP                    1
#define LWIP_UDP                    1

/* ── TCP tuning ──────────────────────────────────────────────────────────── */
#define TCP_MSS                     1460
#define TCP_SND_BUF                 (4 * TCP_MSS)
#define TCP_SND_QUEUELEN            (2 * TCP_SND_BUF / TCP_MSS)
#define TCP_WND                     (4 * TCP_MSS)

/* ── Thread stack / priorities ───────────────────────────────────────────── */
#define TCPIP_THREAD_NAME           "tcp/ip"
#define TCPIP_THREAD_STACKSIZE      512
#define TCPIP_THREAD_PRIO           5       /* above normal tasks */
#define TCPIP_MBOX_SIZE             16
#define DEFAULT_THREAD_STACKSIZE    256
#define DEFAULT_ACCEPTMBOX_SIZE     4
#define DEFAULT_TCP_RECVMBOX_SIZE   8
#define DEFAULT_UDP_RECVMBOX_SIZE   8
#define DEFAULT_RAW_RECVMBOX_SIZE   4

/* ── Misc ────────────────────────────────────────────────────────────────── */
#define LWIP_NETIF_HOSTNAME         1
#define LWIP_STATS                  0
#define LWIP_LOOPIF_MULTITHREADING  1
#define LWIP_TIMEVAL_PRIVATE        0

/* sys_now() ticks from FreeRTOS */
#define LWIP_SYS_NOW_IMPL           1
