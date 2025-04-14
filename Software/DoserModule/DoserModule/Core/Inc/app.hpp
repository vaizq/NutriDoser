/*
 * app.hpp
 *
 *  Created on: Jun 30, 2024
 *      Author: vaige
 */

#ifndef INC_APP_HPP_
#define INC_APP_HPP_

#include "doser_controller.hpp"
#include <cstring>
#include <array>
#include <cmath>
#include <optional>


using namespace std::chrono_literals;
using Doser293KCZL = Doser<60>;

constexpr uint32_t doserCount = 4;

class App
{
public:
    App(CAN_HandleTypeDef& hcan, DRV8874& drv1, DRV8874& drv2);
    void run();
private:
    void filterAll();
    bool initialize();
    bool nextNodeExists();
    void handleRPC(CAN_RxHeaderTypeDef& header, uint8_t data[8]);
    void finishDose(uint32_t doserID);
    void transmitLastNodeCommand();
private:
    CAN_HandleTypeDef& hcan;
    uint32_t moduleID{}; // ID of the first doser. secondDoserID = firstDoserID + 1.
    std::array<Doser293KCZL, doserCount> dosers;
};

void app_main(CAN_HandleTypeDef& hcan);


#endif /* INC_APP_HPP_ */
