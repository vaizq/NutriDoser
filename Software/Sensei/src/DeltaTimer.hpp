#ifndef DELTA_TIMER_HPP
#define DELTA_TIMER_HPP

#include <chrono>

class DeltaTimer {
    using Clock = std::chrono::steady_clock;
public:
    void reset() {
        prev = Clock::now();
    }

    Clock::duration dt() {
        auto now = Clock::now();
        Clock::duration result = now - prev;
        prev = now;
        return result;
    }
private:
    Clock::time_point prev = Clock::now();
};


#endif