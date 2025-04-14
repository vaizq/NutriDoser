/*
 * doser_controller.hpp
 *
 *  Created on: Jun 29, 2024
 *      Author: vaige
 */

#ifndef INC_DOSER_CONTROLLER_HPP_
#define INC_DOSER_CONTROLLER_HPP_

#include "stm32f1xx_hal.h"
#include "clock.h"
#include <functional>
#include <utility>


using namespace std::chrono_literals;

void powerNext();
void shutdownNext();

void terminateCAN();
void removeTerminationCAN();


class GPIOPin
{
public:
	GPIOPin(GPIO_TypeDef* GPIOx, uint16_t pinNumber)
	: GPIOx{GPIOx}, pinNumber(pinNumber)
	{}

	void set()
	{
		HAL_GPIO_WritePin(GPIOx, pinNumber, GPIO_PIN_SET);
	}

	void reset()
	{
		HAL_GPIO_WritePin(GPIOx, pinNumber, GPIO_PIN_RESET);
	}

    void toggle()
    {
        HAL_GPIO_TogglePin(GPIOx, pinNumber);
    }
private:
	GPIO_TypeDef* GPIOx;
	uint16_t pinNumber;
};


class PWMPin
{
public:
	PWMPin(TIM_TypeDef* TIMx, uint32_t channel)
	: TIMx{TIMx}, channel{channel}
	{}

	void setPWM(uint32_t value)
	{
	    switch (channel)
	    {
	        case TIM_CHANNEL_1:
	            TIMx->CCR1 = value;
	            break;
	        case TIM_CHANNEL_2:
	            TIMx->CCR2 = value;
	            break;
	        case TIM_CHANNEL_3:
	            TIMx->CCR3 = value;
	            break;
	        case TIM_CHANNEL_4:
	            TIMx->CCR4 = value;
	            break;
	        default:
	            // Handle error: invalid channel
	            break;
	    }
	}

private:
	TIM_TypeDef* TIMx;
	uint32_t channel;
};


// Application tailored abstraction of the DRV8874 Motor driver
class DRV8874
{
public:
	DRV8874(PWMPin in1, PWMPin in2, GPIOPin nSleep, GPIOPin nFault);

	void out1PWM(uint32_t value);
	void out2PWM(uint32_t value);
	void sleep();
	void wakeup();
	bool isAsleep() const;
	uint32_t readIPropi() const;
	bool isFaulted() const;
private:
	PWMPin in1;
	PWMPin in2;
	GPIOPin nSleep;
	GPIOPin nFault;
};


using FlowRate = uint16_t; // ml/min

// Application tailored abstraction of a doser build on top of DV8874
// This is not a fucking library!
template<FlowRate MaxFlowRate>
class Doser
{
public:
    static constexpr FlowRate maxFlowRate{MaxFlowRate};
    using PwmFn = void (DRV8874::*)(uint32_t value);

	Doser(DRV8874& driver, PwmFn pwmFn)
    :	driver{driver}, pwmFn{pwmFn}
    {
    	stop();
    }

    void run(FlowRate flowRate = maxFlowRate)
    {
    	const uint32_t cntValue = static_cast<uint32_t>(0xFFFF * (1.0f - static_cast<float>(flowRate) / maxFlowRate));
        (driver.*(pwmFn))(cntValue);
    }

    void stop()
    {
    	(driver.*(pwmFn))(0xFFFF);
    }
private:
    DRV8874& driver;
    PwmFn pwmFn;
};


#endif /* INC_DOSER_CONTROLLER_HPP_ */
