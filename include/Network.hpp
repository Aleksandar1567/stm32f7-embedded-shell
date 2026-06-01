#pragma once

// Start the lwIP TCP/IP stack and DHCP.
// Call this once before vTaskStartScheduler().
void Network_Init(void);

// Returns true once DHCP has assigned an IP address.
bool Network_IsUp(void);

// Prints current IP/mask/gw to UART (via dbg).
void Network_PrintInfo(void);
