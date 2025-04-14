#ifndef CAN_DOSER_MANAGER2_HPP
#define CAN_DOSER_MANAGER2_HPP

#include "DoserManager.hpp"
#include "can_protocol.h"
#include "driver/twai.h"
#include "esp_log.h"
#include <cstring>

class CANDoserManager : public DoserManager {
  static constexpr char tag[] = "CANDoserManager";

public:
  CANDoserManager(int parallelMax) : DoserManager{parallelMax} {
    connectDosers();
  }

private:
  std::vector<float> implConnectDosers() override {

    using namespace can::protocol;

    twai_message_t message;
    message.identifier = RPC::Restart;
    message.data_length_code = sizeof(RestartCommand);
    ESP_ERROR_CHECK(twai_transmit(&message, portMAX_DELAY));

    twai_message_t response = {};
    response.identifier = responseID(RPC::NewModule);
    response.data_length_code = sizeof(NewModuleResponse);

    twai_clear_receive_queue();

    for (std::vector<float> flowRates;;) {
      ESP_ERROR_CHECK(twai_receive(&message, portMAX_DELAY));

      switch (message.identifier) {
      case RPC::NewModule: {
        // Parse command
        assert(sizeof(NewModuleCommand) == message.data_length_code);
        NewModuleCommand command{};
        std::memcpy(&command, message.data, sizeof(command));

        // Send response
        const uint32_t moduleID = flowRates.size();
        {
          NewModuleResponse tmp{moduleID};
          std::memcpy(&response.data, &tmp, sizeof(tmp));
        }
        ESP_ERROR_CHECK(twai_transmit(&response, portMAX_DELAY));

        for (int i = 0; i < command.numDosers; ++i) {
          flowRates.push_back(command.maxFlowRate);
        }
        break;
      }
      case RPC::LastNode:
        return flowRates;
      default:
        ESP_LOGE(tag, "Unknown message identifier %lu\n", message.identifier);
        break;
      }
    }

    throw std::runtime_error("connecting to dosers failed!");
  }

  void implDoserOn(int id, float flowRate) {
    using namespace can::protocol;

    twai_message_t message = {};
    message.identifier = RPC::SetFlowRate;
    message.data_length_code = sizeof(SetFlowRateCommand);

    {
      SetFlowRateCommand command{static_cast<uint32_t>(id),
                                 static_cast<uint16_t>(flowRate)};
      std::memcpy(message.data, &command, sizeof(command));
    }

    if (twai_transmit(&message, portMAX_DELAY) != ESP_OK) {
      ESP_LOGE(tag, "Failed to transmit SetFlowRateCommand\n");
    }
  }

  void implDoserOff(int id) { implDoserOn(id, 0); }
};

#endif