//
// Created by vaige on 15.7.2024.
//

#ifndef UNTITLED3_UTIL_H
#define UNTITLED3_UTIL_H

#include <cstdint>
#include <chrono>
#include <random>
#include "Clock.hpp"
#include <concepts>
#include <ArduinoJson.h>


using namespace std::chrono_literals;
using FlowRate = float;

template <std::floating_point T>
static bool epsilonEqual(T lhs, T rhs) {
    return std::abs(lhs - rhs) < std::numeric_limits<T>::epsilon();
}

namespace ArduinoJson {
    template <typename Rep, typename Period>
    struct Converter<std::chrono::duration<Rep, Period>> {
        using StoreDuration = std::chrono::duration<double>;

        static void toJson(const std::chrono::duration<Rep, Period>& duration, JsonVariant dst) {
            dst.set(std::chrono::duration_cast<StoreDuration>(duration).count());
        }
    
        static std::chrono::duration<Rep, Period> fromJson(JsonVariantConst src) {
            return std::chrono::duration_cast<std::chrono::duration<Rep, Period>>(
                StoreDuration(src.as<StoreDuration::rep>()));
        }
    
        static bool check(JsonVariantConst src) {
            return src.is<StoreDuration::rep>();
        }
    };
}

namespace ez {
    template <typename T>
    struct ScopeGuard {
        ScopeGuard(T call) 
        : call{call} {}

        ~ScopeGuard() {
            call();
        }

        ScopeGuard(const ScopeGuard&) = delete;
        ScopeGuard& operator=(const ScopeGuard&) = delete;
    private:
        T call;
    };
}

#endif //UNTITLED3_UTIL_H
