#include "stm32f7xx_hal.h"
#include "stm32f7xx_hal_flash_ex.h"
#include "Led.hpp"
#include "Uart.hpp"
#include "Filesystem.hpp"
#include "Logger.hpp"
#include "Shell.hpp"

extern "C" void SysTick_Handler(void) { HAL_IncTick(); }

// Forward declaration — uart is defined below as a global
extern Uart uart;

extern "C" void HardFault_Handler(void) {
    uart.println("*** HARDFAULT ***");
    while (1) {
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
        HAL_Delay(100);
        HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
        HAL_Delay(100);
    }
}

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

// Global objects
Led  ledGreen(GPIOB, GPIO_PIN_0);
Led  ledBlue (GPIOB, GPIO_PIN_7);
Led  ledRed  (GPIOB, GPIO_PIN_14);
Uart uart(USART3, 115200);
Filesystem fs;
bool fs_mounted = false;

static bool fs_init(void)
{
    Logger::debug("Erasing flash sectors 6+7...");
    HAL_FLASH_Unlock();
    FLASH_EraseInitTypeDef erase = {0};
    erase.TypeErase    = FLASH_TYPEERASE_SECTORS;
    erase.Sector       = FLASH_SECTOR_6;
    erase.NbSectors    = 2;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    uint32_t error_code = 0;
    if (HAL_FLASHEx_Erase(&erase, &error_code) != HAL_OK) {
        Logger::error("Flash erase failed, error_code=%lu", (unsigned long)error_code);
        HAL_FLASH_Lock();
        return false;
    }
    HAL_FLASH_Lock();
    Logger::debug("Flash erase OK");
    return fs.mount();
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    ledGreen.init();
    ledBlue.init();
    ledRed.init();

    uart.init();
    Logger::init();

    Logger::info("System starting");
    fs_mounted = fs_init();

    if (fs_mounted) {
        Logger::info("FS: mounted OK");
    } else {
        Logger::error("FS: mount FAILED");
        ledRed.on();
    }

    // Boot complete — blink all 3 LEDs 3x
    for (int i = 0; i < 3; i++) {
        ledGreen.on(); ledBlue.on(); ledRed.on();
        HAL_Delay(150);
        ledGreen.off(); ledBlue.off(); ledRed.off();
        HAL_Delay(150);
    }

    uart.println("\r\n=== LittleFS Shell ===");
    uart.println("Type 'help' for commands.");

    char line[128];
    while (1) {
        uart.print("\r\n> ");
        int n = uart.readline(line, sizeof(line));
        if (n > 0) {
            uart.print("\r\n");
            parse_cmd(line);
        }
        ledGreen.toggle();
    }
}