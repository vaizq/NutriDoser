//
// Created by vaige on 3.7.2024.
//

#include "error.h"
#include "clock.h"


void printb(uint8_t hb)
{
    if (hb > 0x0F) hb = 0x0F;

    bool ledStates[4] = {};
    const uint32_t pins[4] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};

    uint8_t mask = 0x01;
    for(bool& state : ledStates)
    {
        state = mask & hb;
        mask *= 2;
    }

    for (uint32_t i = 0; i < 4; ++i)
    {
        HAL_GPIO_WritePin(GPIOB, pins[i], ledStates[i] ? GPIO_PIN_SET : GPIO_PIN_RESET);
    }
}

void handleError(uint8_t err)
{
    using namespace std::chrono_literals;

    for (;;)
    {
        printb(err);
        ez::delay(2s);
        printb(0);
        ez::delay(2s);
    }
}

void fatal()
{
    handleError();
}
