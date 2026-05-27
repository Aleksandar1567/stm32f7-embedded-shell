#include "stm32f7xx_hal.h"
#include "Led.hpp"
#include "Uart.hpp"
#include "Filesystem.hpp"
#include <string.h>

extern "C" void SysTick_Handler(void) { HAL_IncTick(); }

static void SystemClock_Config(void)
{
    RCC_OscInitTypeDef osc = {0};
    osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
    osc.HSIState            = RCC_HSI_ON;
    osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
    osc.PLL.PLLState        = RCC_PLL_ON;
    osc.PLL.PLLSource       = RCC_PLLSOURCE_HSI;
    osc.PLL.PLLM = 8; osc.PLL.PLLN = 216;
    osc.PLL.PLLP = RCC_PLLP_DIV2;
    osc.PLL.PLLQ = 9; osc.PLL.PLLR = 7;
    HAL_RCC_OscConfig(&osc);
    HAL_PWREx_EnableOverDrive();
    RCC_ClkInitTypeDef clk = {0};
    clk.ClockType      = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK |
                         RCC_CLOCKTYPE_PCLK1  | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource   = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider  = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV4;
    clk.APB2CLKDivider = RCC_HCLK_DIV2;
    HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_7);
}

// Globalni objekti
Led  ledGreen(GPIOB, GPIO_PIN_0);
Led  ledBlue (GPIOB, GPIO_PIN_7);
Led  ledRed  (GPIOB, GPIO_PIN_14);
Uart uart(USART3, 115200);
Filesystem fs;

static void parse_cmd(char *line)
{
    char out[512];

    if (strcmp(line, "ls") == 0) {
        fs.ls(out, sizeof(out));
        uart.print(out);
        return;
    }
    if (strncmp(line, "read ", 5) == 0) {
        if (fs.read(line + 5, out, sizeof(out)))
            uart.println(out);
        else
            uart.println("ERR: file not found");
        return;
    }
    if (strncmp(line, "rm ", 3) == 0) {
        uart.println(fs.remove(line + 3) ? "OK" : "ERR: cannot remove");
        return;
    }
    if (strncmp(line, "write ", 6) == 0) {
        char *rest  = line + 6;
        char *space = strchr(rest, ' ');
        if (!space) { uart.println("Usage: write <file> <data>"); return; }
        *space = '\0';
        uart.println(fs.write(rest, space + 1) ? "OK" : "ERR: cannot write");
        return;
    }
    if (strcmp(line, "help") == 0) {
        uart.println("Commands:");
        uart.println("  ls");
        uart.println("  read <file>");
        uart.println("  write <file> <data>");
        uart.println("  rm <file>");
        return;
    }
    uart.println("Unknown command. Type 'help'.");
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    ledGreen.init();
    ledBlue.init();
    ledRed.init();
    uart.init();

    uart.println("\r\n=== LittleFS Shell ===");

    if (!fs.mount()) {
        uart.println("ERR: filesystem mount failed!");
        ledRed.on();
        while (1) {}
    }
    uart.println("Filesystem OK. Type 'help'.");

    char line[128];
    while (1) {
        uart.print("\r\n> ");
        int n = uart.readline(line, sizeof(line));
        if (n > 0) {
            uart.print("\r\n");
            parse_cmd(line);
        }
        ledGreen.toggle(); // znak da je živ
    }
}