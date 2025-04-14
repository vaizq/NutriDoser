//
// Created by vaige on 20.7.2024.
//

#ifndef SENSEI_PID_H
#define SENSEI_PID_H

#include <optional>
#include <chrono>


class Pid
{
public:
    Pid(float kp, float ki, float kd)
    : kp{kp}, ki{ki}, kd{kd}
    {}

    float update(float input, std::chrono::duration<float, std::chrono::seconds::period> dt)
    {
        const float proportional = kp * input;
        integral += ki * input * dt.count();
        const float derivative = kd * (input - prevInput) / dt.count();
        prevInput = input;
        return proportional + integral + derivative;
    }

    float kp;
    float ki;
    float kd;
private:
    float integral{0};
    float prevInput{0};
};


#endif //SENSEI_PID_H
