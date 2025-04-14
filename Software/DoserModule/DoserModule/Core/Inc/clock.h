//
// Created by vaige on 1.7.2024.
//

#ifndef NUTRIDOSERSOFTWARE_CLOCK_H
#define NUTRIDOSERSOFTWARE_CLOCK_H

#include <chrono>
#include "stm32f1xx_hal.h"


namespace ez
{

    namespace detail
    {
        extern uint64_t clkCnt;
    }

    struct Clock
    {
        using duration = std::chrono::microseconds;
        using rep = duration::rep;
        using period = duration::period;
        using time_point = std::chrono::time_point<Clock, duration>;

        static constexpr bool is_steady = true;

        static time_point now() noexcept;
    };

    void delay(Clock::duration duration);


}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim);


#endif //NUTRIDOSERSOFTWARE_CLOCK_H
