#pragma once

#include "stm32f7xx_hal.h"

// Global ETH handle — used by lwIP netif in a later step
extern ETH_HandleTypeDef heth;

// Initialize Ethernet peripheral (GPIO, clocks, ETH HAL, LAN8742 PHY).
// Must be called after HAL_Init() and SystemClock_Config(), before RTOS start.
// Returns true on success.
bool ETH_Init(void);

// Returns true if the cable is plugged in and autoneg completed.
bool ETH_IsLinkUp(void);
