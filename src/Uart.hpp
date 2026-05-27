#pragma once
#include "stm32f7xx_hal.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

class Uart {
public:
    Uart(USART_TypeDef *instance, uint32_t baud)
        : baud_(baud)
    {
        handle_.Instance = instance;
    }

    void init() {
        __HAL_RCC_USART3_CLK_ENABLE();
        __HAL_RCC_GPIOD_CLK_ENABLE();

        GPIO_InitTypeDef gpio = {0};
        gpio.Pin       = GPIO_PIN_8 | GPIO_PIN_9;
        gpio.Mode      = GPIO_MODE_AF_PP;
        gpio.Pull      = GPIO_NOPULL;
        gpio.Speed     = GPIO_SPEED_FREQ_LOW;
        gpio.Alternate = GPIO_AF7_USART3;
        HAL_GPIO_Init(GPIOD, &gpio);

        handle_.Init.BaudRate   = baud_;
        handle_.Init.WordLength = UART_WORDLENGTH_8B;
        handle_.Init.StopBits   = UART_STOPBITS_1;
        handle_.Init.Parity     = UART_PARITY_NONE;
        handle_.Init.Mode       = UART_MODE_TX_RX;
        handle_.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
        HAL_UART_Init(&handle_);
    }

    void print(const char *s) {
        HAL_UART_Transmit(&handle_, (uint8_t *)s, strlen(s), 1000);
    }

    void println(const char *s) {
        print(s);
        print("\r\n");
    }

    void printf(const char *fmt, ...) {
        char buf[256];
        va_list args;
        va_start(args, fmt);
        vsnprintf(buf, sizeof(buf), fmt, args);
        va_end(args);
        print(buf);
    }

    // Čita jednu liniju, vraća dužinu
    int readline(char *buf, int maxlen, uint32_t timeout_ms = 5000) {
        int i = 0;
        uint8_t c;
        while (i < maxlen - 1) {
            if (HAL_UART_Receive(&handle_, &c, 1, timeout_ms) != HAL_OK) break;
            HAL_UART_Transmit(&handle_, &c, 1, 100); // echo
            if (c == '\r' || c == '\n') break;
            buf[i++] = c;
        }
        buf[i] = '\0';
        return i;
    }

private:
    UART_HandleTypeDef handle_;
    uint32_t           baud_;
};