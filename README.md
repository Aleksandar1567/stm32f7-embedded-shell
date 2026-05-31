# stm32f7-littlefs-shell

Bare-metal firmware for the STM32F767ZI (NUCLEO-F767ZI) that exposes an interactive shell over UART, backed by LittleFS for persistent file storage on internal flash. Built with PlatformIO.

> **Branch `feature/freertos`** — FreeRTOS kernel added; LED tasks run as independent RTOS tasks.

## Features

- Interactive UART shell at 115200 baud — works with any serial terminal
- LittleFS filesystem mounted on internal flash sectors 6 & 7 with wear levelling and power-loss resilience
- Shell commands: `ls`, `read`, `write`, `rm`, `help`, `logs`, `logs clear`
- Non-blocking circular-buffer logger (4 KB) with `[ERROR]` / `[WARN]` / `[INFO]` / `[DEBUG]` levels and HAL timestamps
- **FreeRTOS 10.3.1** — preemptive multitasking, three LED tasks with independent periods
- RGB LED tasks: green 500 ms, blue 500 ms, red 500 ms — each as a separate FreeRTOS task
- `TASK_LOG_ENABLED` compile-time switch — zero overhead when disabled (strings stripped from Flash)
- HardFault handler that prints a message and blinks the red LED
- Runs at 216 MHz via HSI PLL with Over-Drive enabled (FLASH_LATENCY_7)
- C++17, STM32 HAL framework, LittleFS and FreeRTOS fetched automatically by PlatformIO

## Hardware

| Resource | Assignment |
|---|---|
| Board | NUCLEO-F767ZI |
| MCU | STM32F767ZI |
| UART | USART3 @ 115200 8N1 (PD8 TX, PD9 RX) |
| LED green | PB0 |
| LED blue | PB7 |
| LED red | PB14 |
| Flash storage | Sectors 6 & 7 (internal flash) |

## FreeRTOS integration

### Interrupt wiring

The FreeRTOS port functions are mapped to the Cortex-M vector table names via `FreeRTOSConfig.h`:

```c
#define vPortSVCHandler    SVC_Handler
#define xPortPendSVHandler PendSV_Handler
```

`SysTick_Handler` calls `HAL_IncTick()` directly and forwards to `xPortSysTickHandler()` only after the scheduler has started, avoiding a crash during early boot:

```c
void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED)
        xPortSysTickHandler();
}
```

### Task logging

Task-level UART prints can be toggled at compile time:

```c
// src/main.cpp
#define TASK_LOG_ENABLED 1   // debug — full UART output
#define TASK_LOG_ENABLED 0   // production — zero overhead, no strings in Flash
```

### FreeRTOS config highlights (`include/FreeRTOSConfig.h`)

| Parameter | Value |
|---|---|
| `configCPU_CLOCK_HZ` | 216 000 000 |
| `configTICK_RATE_HZ` | 1000 (1 ms tick) |
| `configTOTAL_HEAP_SIZE` | 32 KB |
| `configMINIMAL_STACK_SIZE` | 256 words |
| Tick hook | disabled |
| Idle hook | disabled |

## Shell commands

```
> help
Commands:
  logs              - show all logs
  logs clear        - clear logs
  ls                - list files
  read <file>       - read file
  write <file> <data>
  rm <file>         - remove file
```

## Project structure

```
├── include/
│   ├── Filesystem.hpp   # LittleFS wrapper
│   ├── FreeRTOSConfig.h # FreeRTOS configuration
│   ├── Led.hpp          # GPIO LED abstraction
│   ├── Logger.hpp       # Circular-buffer logger
│   ├── Shell.hpp        # Command parser
│   └── Uart.hpp         # USART blocking driver
├── lib/
│   └── FreeRTOS/        # FreeRTOS 10.3.1 (Cortex-M4F/M7 port)
├── src/
│   ├── Filesystem.cpp
│   ├── Logger.cpp
│   ├── Shell.cpp
│   └── main.cpp         # Entry point, clock config, RTOS tasks
├── platformio.ini
└── README.md
```

## PlatformIO setup

Install [PlatformIO](https://platformio.org/install) (VS Code extension or CLI). All dependencies (STM32 HAL, LittleFS, FreeRTOS) are resolved automatically on first build.

### Build

```bash
pio run
```

### Build & flash via ST-Link

```bash
pio run -t upload
```

### Open serial monitor (115200 baud)

```bash
pio device monitor --baud 115200
```

### Other useful commands

```bash
# Clean build artifacts
pio run -t clean

# List detected serial ports
pio device list

# Build + upload + open monitor in one shot
pio run -t upload && pio device monitor --baud 115200

# Show verbose build output
pio run -v

# Check / update platform and library dependencies
pio pkg update
```

## Logger

The logger stores messages in a 4 KB circular buffer without blocking the main loop. To see accumulated logs, type `logs` in the shell. To discard them, type `logs clear`.

Log format: `[LEVEL][<ms since boot>] message`

## License

MIT


Bare-metal firmware for the STM32F767ZI (NUCLEO-F767ZI) that exposes an interactive shell over UART, backed by LittleFS for persistent file storage on internal flash. Built with PlatformIO.

## Features

- Interactive UART shell at 115200 baud — works with any serial terminal
- LittleFS filesystem mounted on internal flash sectors 6 & 7 with wear levelling and power-loss resilience
- Shell commands: `ls`, `read`, `write`, `rm`, `help`, `logs`, `logs clear`
- Non-blocking circular-buffer logger (4 KB) with `[ERROR]` / `[WARN]` / `[INFO]` / `[DEBUG]` levels and HAL timestamps
- RGB LED status indicators: green heartbeat blink, red on fatal error / mount failure, all three blink 3× on boot
- HardFault handler that prints a message and blinks the red LED
- Runs at 216 MHz via HSI PLL with Over-Drive enabled (FLASH_LATENCY_7)
- C++17, STM32 HAL framework, LittleFS fetched automatically by PlatformIO

## Hardware

| Resource | Assignment |
|---|---|
| Board | NUCLEO-F767ZI |
| MCU | STM32F767ZI |
| UART | USART3 @ 115200 8N1 |
| LED green | PB0 |
| LED blue | PB7 |
| LED red | PB14 |
| Flash storage | Sectors 6 & 7 (internal flash) |

## Shell commands

```
> help
Commands:
  logs              - show all logs
  logs clear        - clear logs
  ls                - list files
  read <file>       - read file
  write <file> <data>
  rm <file>         - remove file
```

## Project structure

```
├── include/
│   ├── Filesystem.hpp   # LittleFS wrapper
│   ├── Led.hpp          # GPIO LED abstraction
│   ├── Logger.hpp       # Circular-buffer logger
│   ├── Shell.hpp        # Command parser
│   └── Uart.hpp         # USART blocking driver
├── src/
│   ├── Filesystem.cpp
│   ├── Logger.cpp
│   ├── Shell.cpp
│   └── main.cpp         # Entry point, clock config, shell loop
├── platformio.ini
└── README.md
```

## PlatformIO setup

Install [PlatformIO](https://platformio.org/install) (VS Code extension or CLI). All dependencies (STM32 HAL, LittleFS) are resolved automatically on first build.

### Build

```bash
pio run
```

### Build & flash via ST-Link

```bash
pio run -t upload
```

### Open serial monitor (115200 baud)

```bash
pio device monitor --baud 115200
```

### Other useful commands

```bash
# Clean build artifacts
pio run -t clean

# List detected serial ports
pio device list

# Build + upload + open monitor in one shot
pio run -t upload && pio device monitor --baud 115200

# Show verbose build output
pio run -v

# Check / update platform and library dependencies
pio pkg update
```

## Logger

The logger stores messages in a 4 KB circular buffer without blocking the main loop. To see accumulated logs, type `logs` in the shell. To discard them, type `logs clear`.

Log format: `[LEVEL][<ms since boot>] message`

## License

MIT
