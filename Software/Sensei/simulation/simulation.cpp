//
// Created by vaige on 20.7.2024.
//
#include <iostream>
#include "../src/Pid.h"
#include <iomanip>

class Reservoir
{
private:
    float liquid_amount;
    float ph;
    float ec;
public:
    Reservoir(float liquid_amount, float ph, float ec)
    : liquid_amount{liquid_amount}, ph{ph}, ec{ec}
    {}

    void add_water(float amount)
    {
        liquid_amount += amount;
    }

    void remove_liquid(float amount)
    {
        liquid_amount -= amount;
    }

    void add_ph_down(float amount)
    {
        // Suppose that 1ml / 10000ml makes ph go down by 1
        const float ratio = amount / liquid_amount;
        ph -= ratio  * 10000.0f;
    }

    void add_ph_up(float amount)
    {
        const float ratio = amount / liquid_amount;
        ph += ratio  * 10000.0f;
    }

    void add_nutrient(float amount)
    {

    }

    float get_liquid_amount() const { return liquid_amount; }
    float get_ph() const { return ph; }
    float get_ec() const { return ec; }
};

using namespace std::chrono_literals;

int main(int argc, char** argv)
{
    Reservoir reservoir{250 * 1000, 10.0f, 0.2f};
    constexpr float target = 5.8f;
    Pid pid{reservoir.get_liquid_amount() / 1000.0f / 20.0f, 0.000f, 0.0f};

    int count{0};
    while (std::abs(reservoir.get_ph() - target) > 0.1f)
    {
        const float err = target - reservoir.get_ph();
        float result = pid.update(err, 60s);
        std::cout << "ph: " << std::fixed << std::setprecision(3) << std::setw(5) << reservoir.get_ph()
                  << " add: " << std::fixed << std::setprecision(3) << std::setw(5) << std::abs(result) << "ml of " << ((result > 0.0f) ? "ph-up" : "ph-down") << std::endl;
        if (result > 0)
        {
            reservoir.add_ph_up(std::abs(result));
        }
        else
        {
            reservoir.add_ph_down(std::abs(result));
        }
        ++count;
    }
    std::cout << "PH adjusted in " << count << "-minutes\n";
}