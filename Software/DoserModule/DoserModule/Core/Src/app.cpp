//
// Created by vaige on 3.7.2024.
//
#include "app.hpp"
#include "error.h"
#include "protocol.h"


enum Err
{
    Init = 3,
    Command = 5,
};

void app_main(CAN_HandleTypeDef& hcan)
{
    DRV8874 drv1{
            {TIM2, TIM_CHANNEL_2},
            {TIM2, TIM_CHANNEL_1},
            {GPIOA, GPIO_PIN_4},
            {GPIOA, GPIO_PIN_5}
    };

    DRV8874 drv2{
            {TIM2, TIM_CHANNEL_4},
            {TIM2, TIM_CHANNEL_3},
            {GPIOA, GPIO_PIN_6},
            {GPIOA, GPIO_PIN_7}
    };

    App app{hcan, drv1, drv2};
    app.run();
}


App::App(CAN_HandleTypeDef &hcan, DRV8874 &drv1, DRV8874 &drv2)
:   hcan{hcan},
    dosers{
        Doser293KCZL{drv1, &DRV8874::out1PWM},
        Doser293KCZL{drv1, &DRV8874::out2PWM},
        Doser293KCZL{drv2, &DRV8874::out1PWM},
        Doser293KCZL{drv2, &DRV8874::out2PWM}
    }
{
	filterAll();
    if (!initialize())
    {
        handleError(Err::Init);
    }

    // Try to power up further nodes
    powerNext();
    removeTerminationCAN();
    if (!nextNodeExists())
    {
        shutdownNext();
        terminateCAN();
        transmitLastNodeCommand();
    }
}

void App::run()
{
    CAN_RxHeaderTypeDef rxHeader;

    for (;;)
    {
        if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0)
        {
            uint8_t rxData[8];
            if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
            {
                handleRPC(rxHeader, rxData);
            }
        }

        ez::delay(1ms);
    }
}

bool App::initialize()
{
	using namespace can::protocol;

    NewModuleCommand command{doserCount, Doser293KCZL::maxFlowRate};
    const CAN_TxHeaderTypeDef initHeader = {
            .StdId = RPC::NewModule,
            .IDE = CAN_ID_STD,
            .RTR = CAN_RTR_DATA,
            .DLC = sizeof(command)
    };
    uint32_t mailbox;
    uint8_t txData[8] = {};
    std::memcpy(txData, &command, sizeof(command));

    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];

    constexpr int retryMax = 10000;
    for(int i = 0; i < retryMax; ++i)
    {
    	while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan) == 0) { ez::delay(10ms); }

        if (HAL_CAN_AddTxMessage(&hcan, &initHeader, txData, &mailbox) != HAL_OK)
        {
            handleError();
        }

        // Wait for the response to arrive
        const auto listenStart = ez::Clock::now();
        uint32_t fillLevel;
        while ((fillLevel = HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0)) == 0 &&
               (ez::Clock::now() - listenStart < 500ms))
        {}

        while ((fillLevel-- > 0) &&
               HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
        {
            if (rxHeader.StdId == responseID(RPC::NewModule))
            {
                NewModuleResponse response;
                std::memcpy(&response, rxData, sizeof(response));
                moduleID = response.moduleID;
                return true;
            }
        }
    }
    return false;
}

bool App::nextNodeExists()
{
    CAN_RxHeaderTypeDef rxHeader;
    uint8_t rxData[8];
    const auto startTime = ez::Clock::now();

    // Wait 1s for a NewModule message
    while (ez::Clock::now() - startTime < 500ms)
    {
        if (HAL_CAN_GetRxFifoFillLevel(&hcan, CAN_RX_FIFO0) > 0)
        {
            if (HAL_CAN_GetRxMessage(&hcan, CAN_RX_FIFO0, &rxHeader, rxData) == HAL_OK)
            {
                if (rxHeader.StdId == can::protocol::RPC::NewModule)
                {
                    return true;
                }
            }
        }
    }

    return false;
}

void App::transmitLastNodeCommand()
{
	using namespace can::protocol;

	const CAN_TxHeaderTypeDef initHeader = {
			.StdId = RPC::LastNode,
			.IDE = CAN_ID_STD,
			.RTR = CAN_RTR_DATA,
			.DLC = sizeof(LastNodeCommand)
	};
	uint32_t mailbox;
	uint8_t txData[8] = {};

	if (HAL_CAN_AddTxMessage(&hcan, &initHeader, txData, &mailbox) != HAL_OK)
	{
		handleError();
	}
}

void App::filterAll()
{
    CAN_FilterTypeDef sFilterConfig;
    sFilterConfig.FilterBank = 0;
    sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;
    sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;
    sFilterConfig.FilterIdHigh = 0x0000;
    sFilterConfig.FilterIdLow = 0x0000;
    sFilterConfig.FilterMaskIdHigh = 0x0000;
    sFilterConfig.FilterMaskIdLow = 0x0000;
    sFilterConfig.FilterFIFOAssignment = CAN_RX_FIFO0;
    sFilterConfig.FilterActivation = ENABLE;

    if (HAL_CAN_ConfigFilter(&hcan, &sFilterConfig) != HAL_OK)
    {
        handleError();
    }
}

void App::handleRPC(CAN_RxHeaderTypeDef &header, uint8_t *data)
{
	if (header.StdId == can::protocol::RPC::SetFlowRate)
	{
		can::protocol::SetFlowRateCommand command;
		std::memcpy(&command, data, sizeof(command));

		const uint32_t idx = command.doserID - moduleID;
		if (idx < dosers.size())
		{
			if (command.flowRate == 0)
			{
				dosers[idx].stop();
			}
			else
			{
				dosers[idx].run(command.flowRate);
			}
		}
	} else if (header.StdId == can::protocol::RPC::Restart) {
		shutdownNext();
		terminateCAN();
		for (auto& doser : dosers) {
			doser.stop();
		}
		HAL_NVIC_SystemReset();
	}
}
