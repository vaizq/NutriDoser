/*
 * doser_controller.cpp
 *
 *  Created on: Jun 29, 2024
 *      Author: vaige
 */

#include "doser_controller.hpp"

// Provide power to the next node in the chain
void powerNext()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
}

// Cut power from the next node in the chain
void shutdownNext()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
}

// Terminate the can bus at this node
void terminateCAN()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
}

// Open termination of the can bus at this node
void removeTerminationCAN()
{
	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_8, GPIO_PIN_SET);
}


DRV8874::DRV8874(PWMPin in1, PWMPin in2, GPIOPin nSleep, GPIOPin nFault)
: in1{in1}, in2{in2}, nSleep{nSleep}, nFault{nFault}
{
	wakeup();
    in1.setPWM(0xFFFF);
    in2.setPWM(0xFFFF);
}

void DRV8874::out1PWM(uint32_t value)
{
	in1.setPWM(value);
}

void DRV8874::out2PWM(uint32_t value)
{
	in2.setPWM(value);
}

void DRV8874::sleep()
{
	nSleep.reset();
}

void DRV8874::wakeup()
{
	nSleep.set();
}

bool DRV8874::isAsleep() const
{
	return false;
}

uint32_t DRV8874::readIPropi() const
{
	return 0;
}

bool DRV8874::isFaulted() const
{
	return false;
}
