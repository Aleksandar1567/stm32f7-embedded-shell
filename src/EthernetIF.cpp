// src/EthernetIF.cpp
// Step 1 — Ethernet HAL init + LAN8742 PHY init on NUCLEO-F767ZI (RMII)
//
// RMII pin mapping (UM1974 — NUCLEO-F767ZI user manual):
//   PA1  → ETH_RMII_REF_CLK  (AF11)
//   PA2  → ETH_MDIO           (AF11)
//   PA7  → ETH_RMII_CRS_DV   (AF11)
//   PB13 → ETH_RMII_TXD1     (AF11)
//   PC1  → ETH_MDC            (AF11)
//   PC4  → ETH_RMII_RXD0     (AF11)
//   PC5  → ETH_RMII_RXD1     (AF11)
//   PG11 → ETH_RMII_TX_EN    (AF11)
//   PG13 → ETH_RMII_TXD0     (AF11)

#include "EthernetIF.hpp"
#include "lan8742.h"
#include <cstring>

// ── DMA descriptors — must live in DMA-accessible SRAM ──────────────────────
__ALIGN_BEGIN static ETH_DMADescTypeDef DMARxDscrTab[ETH_RX_DESC_CNT] __ALIGN_END;
__ALIGN_BEGIN static ETH_DMADescTypeDef DMATxDscrTab[ETH_TX_DESC_CNT] __ALIGN_END;

// ── HAL Ethernet handle (extern in EthernetIF.hpp) ──────────────────────────
ETH_HandleTypeDef heth;

// ── LAN8742 PHY object ───────────────────────────────────────────────────────
static lan8742_Object_t LAN8742;

// ── PHY IO wrappers — bridge LAN8742 driver to HAL_ETH MDIO ─────────────────
static int32_t PHY_IO_Init(void)   { return 0; }
static int32_t PHY_IO_DeInit(void) { return 0; }

static int32_t PHY_IO_WriteReg(uint32_t devAddr, uint32_t reg, uint32_t val)
{
    return (HAL_ETH_WritePHYRegister(&heth, devAddr, reg, val) == HAL_OK) ? 0 : -1;
}

static int32_t PHY_IO_ReadReg(uint32_t devAddr, uint32_t reg, uint32_t *val)
{
    return (HAL_ETH_ReadPHYRegister(&heth, devAddr, reg, val) == HAL_OK) ? 0 : -1;
}

static int32_t PHY_IO_GetTick(void) { return static_cast<int32_t>(HAL_GetTick()); }

// ── HAL_ETH_MspInit — GPIO + clock init, called automatically by HAL_ETH_Init
extern "C" void HAL_ETH_MspInit(ETH_HandleTypeDef * /*heth_ptr*/)
{
    GPIO_InitTypeDef gpio = {};

    // 1. Enable peripheral clocks
    __HAL_RCC_ETH_CLK_ENABLE();       // ETHMAC + ETHMACTX + ETHMACRX
    __HAL_RCC_GPIOA_CLK_ENABLE();
    __HAL_RCC_GPIOB_CLK_ENABLE();
    __HAL_RCC_GPIOC_CLK_ENABLE();
    __HAL_RCC_GPIOG_CLK_ENABLE();

    // 2. Common RMII GPIO settings
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_VERY_HIGH;
    gpio.Alternate = GPIO_AF11_ETH;

    // PA1 = REF_CLK, PA2 = MDIO, PA7 = CRS_DV
    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_2 | GPIO_PIN_7;
    HAL_GPIO_Init(GPIOA, &gpio);

    // PB13 = TXD1
    gpio.Pin = GPIO_PIN_13;
    HAL_GPIO_Init(GPIOB, &gpio);

    // PC1 = MDC, PC4 = RXD0, PC5 = RXD1
    gpio.Pin = GPIO_PIN_1 | GPIO_PIN_4 | GPIO_PIN_5;
    HAL_GPIO_Init(GPIOC, &gpio);

    // PG11 = TX_EN, PG13 = TXD0
    gpio.Pin = GPIO_PIN_11 | GPIO_PIN_13;
    HAL_GPIO_Init(GPIOG, &gpio);
}

// ── Public API ───────────────────────────────────────────────────────────────

bool ETH_Init(void)
{
    static const uint8_t mac[6] = {0x02, 0x00, 0x00, 0x00, 0x00, 0x01};

    heth.Instance            = ETH;
    std::memcpy(heth.Init.MACAddr, mac, sizeof(mac));
    heth.Init.MediaInterface = HAL_ETH_RMII_MODE;
    heth.Init.TxDesc         = DMATxDscrTab;
    heth.Init.RxDesc         = DMARxDscrTab;
    heth.Init.RxBuffLen      = ETH_MAX_PACKET_SIZE;  // 1528

    if (HAL_ETH_Init(&heth) != HAL_OK) {
        return false;
    }

    // Register PHY IO and init LAN8742
    lan8742_IOCtx_t phyIO = {
        PHY_IO_Init,
        PHY_IO_DeInit,
        PHY_IO_WriteReg,
        PHY_IO_ReadReg,
        PHY_IO_GetTick
    };

    if (LAN8742_RegisterBusIO(&LAN8742, &phyIO) != LAN8742_STATUS_OK) {
        return false;
    }

    if (LAN8742_Init(&LAN8742) != LAN8742_STATUS_OK) {
        return false;
    }

    // Start ETH DMA transmit/receive
    if (HAL_ETH_Start(&heth) != HAL_OK) {
        return false;
    }

    return true;
}

bool ETH_IsLinkUp(void)
{
    int32_t state = LAN8742_GetLinkState(&LAN8742);
    // > LINK_DOWN means 100FD / 100HD / 10FD / 10HD
    return state > LAN8742_STATUS_LINK_DOWN;
}
