#pragma once
#include "stm32f7xx_hal.h"

class Led {
public:
    Led(GPIO_TypeDef *port, uint16_t pin) : port_(port), pin_(pin) {}

    void init() {
        // Enable GPIO port clock
        if (port_ == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
        if (port_ == GPIOC) __HAL_RCC_GPIOC_CLK_ENABLE();
        // Add more ports as needed

        GPIO_InitTypeDef g = {0};
        g.Pin   = pin_;
        g.Mode  = GPIO_MODE_OUTPUT_PP;
        g.Pull  = GPIO_NOPULL;
        g.Speed = GPIO_SPEED_FREQ_LOW;
        HAL_GPIO_Init(port_, &g);
        off();
    }

    void on()     { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_SET);   }
    void off()    { HAL_GPIO_WritePin(port_, pin_, GPIO_PIN_RESET); }
    void toggle() { HAL_GPIO_TogglePin(port_, pin_); }

private:
    GPIO_TypeDef *port_;
    uint16_t      pin_;
};