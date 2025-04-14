//
// Created by vaige on 1.7.2024.
//
#include "clock.h"

namespace ez
{

    namespace detail
    {
        uint64_t clkCnt{0};
    }

        Clock::time_point Clock::now() noexcept
        {
            return time_point(duration(detail::clkCnt + TIM1->CNT));
        }

        void delay(Clock::duration duration)
        {
            const auto t0 = Clock::now();
            while (Clock::now() - t0 < duration) {}
        }

}

extern "C" void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef* htim)
{
    if (htim->Instance == TIM1)
    {
        ez::detail::clkCnt += 65536; // Number of clock ticks
    }
}