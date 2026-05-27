# stm32f7-littlefs-shell

Bare-metal firmware for the STM32F7 series that exposes a simple interactive shell over UART, backed by LittleFS for persistent file storage on internal flash.

## Features

- Interactive UART shell at 115200 baud — works with any serial terminal (PuTTY, minicom, screen)
- LittleFS filesystem mounted on internal flash with wear levelling and power-loss resilience
- Commands: `ls`, `read`, `write`, `rm`, `help`
- RGB LED status indicators (green heartbeat, red on fatal error)
- Runs at 216 MHz via HSI PLL with Over-Drive enabled

## Hardware

| Resource | Assignment |
|---|---|
| MCU | STM32F7xx |
| UART | USART3 @ 115200 8N1 |
| LED green | PB0 |
| LED blue | PB7 |
| LED red | PB14 |

## Shell commands

```
> help
Commands:
  ls
  read <file>
  write <file> <data>
  rm <file>
```

## Project structure

```
├── Core/
│   ├── main.cpp          # Entry point, shell loop
│   ├── Led.hpp/.cpp      # GPIO LED abstraction
│   ├── Uart.hpp/.cpp     # USART blocking driver
│   └── Filesystem.hpp/.cpp  # LittleFS wrapper
└── Drivers/
    └── ...               # STM32 HAL + LittleFS
```

## Building

Requires STM32CubeIDE or an ARM GCC toolchain with the STM32F7 HAL and LittleFS sources added to the project.

1. Clone the repo and open in STM32CubeIDE, or configure your `CMakeLists.txt` / Makefile to include the HAL and LittleFS sources.
2. Set the target device to your exact STM32F7 variant and adjust flash region parameters in `Filesystem.cpp` if needed.
3. Build and flash via ST-Link.

## License

MIT
