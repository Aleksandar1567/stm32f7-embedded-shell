#include "stm32f7xx_hal.h"
#include "Led.hpp"
#include "Uart.hpp"
#include "EthernetIF.hpp"
#include "FreeRTOS.h"
#include "task.h"

// Turn on/off task logging (over UART) - can be useful for debugging scheduler issues, but adds overhead
#define TASK_LOG_ENABLED 0

#if TASK_LOG_ENABLED
  #define TASK_LOG(msg)        dbg.println(msg)
  #define TASK_LOGF(fmt, ...)  dbg.printf(fmt, ##__VA_ARGS__)
#else
  #define TASK_LOG(msg)        ((void)0)
  #define TASK_LOGF(fmt, ...)  ((void)0)
#endif

extern "C" void xPortSysTickHandler(void);

extern "C" void SysTick_Handler(void) {
    HAL_IncTick();
    if (xTaskGetSchedulerState() != taskSCHEDULER_NOT_STARTED) {
        xPortSysTickHandler();
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
Uart dbg(USART3, 115200);
Led  ledGreen(GPIOB, GPIO_PIN_0);
Led  ledBlue (GPIOB, GPIO_PIN_7);
Led  ledRed  (GPIOB, GPIO_PIN_14);

static void greenTask(void*) {
    TASK_LOG("[greenTask] started");
    for (;;) {
        ledGreen.toggle();
        TASK_LOG("[greenTask] toggle");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void blueTask(void*) {
    TASK_LOG("[blueTask] started");
    for (;;) {
        ledBlue.toggle();
        TASK_LOG("[blueTask] toggle");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

static void redTask(void*) {
    TASK_LOG("[redTask] started");
    for (;;) {
        ledRed.toggle();
        TASK_LOG("[redTask] toggle");
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

int main(void)
{
    HAL_Init();
    SystemClock_Config();

    dbg.init();
    dbg.println("[main] boot OK");

    ledGreen.init();
    ledBlue.init();
    ledRed.init();
    dbg.println("[main] LEDs init OK");

    if (ETH_Init()) {
        dbg.println("[main] ETH init OK");
        dbg.printf("[main] link: %s\r\n", ETH_IsLinkUp() ? "UP" : "DOWN");
    } else {
        dbg.println("[main] ETH init FAILED");
    }

    BaseType_t r1 = xTaskCreate(greenTask, "Green", 128, nullptr, 1, nullptr);
    BaseType_t r2 = xTaskCreate(blueTask,  "Blue",  128, nullptr, 1, nullptr);
    BaseType_t r3 = xTaskCreate(redTask,   "Red",   128, nullptr, 1, nullptr);

    dbg.printf("[main] tasks created: green=%d blue=%d red=%d\r\n", r1, r2, r3);

    dbg.println("[main] starting scheduler");
    vTaskStartScheduler();
    dbg.println("[main] ERROR: scheduler returned!");
    for (;;);
}