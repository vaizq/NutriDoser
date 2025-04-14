//
// Created by vaige on 3.7.2024.
//

#ifndef NUTRIDOSERSOFTWARE_ERROR_H
#define NUTRIDOSERSOFTWARE_ERROR_H

#include "stm32f1xx_hal.h"


void printb(uint8_t hb);
void handleError(uint8_t err = 0xFF);
void fatal();


#endif //NUTRIDOSERSOFTWARE_ERROR_H
