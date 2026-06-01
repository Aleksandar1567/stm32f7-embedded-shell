#pragma once

// Start TCP echo server on port 7 (runs as FreeRTOS task).
// Call after Network_Init(), before vTaskStartScheduler().
void TcpServer_Start(void);
