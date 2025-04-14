//
// Created by vaige on 3.7.2024.
//

#ifndef NUTRIDOSERSOFTWARE_PROTOCOL_H
#define NUTRIDOSERSOFTWARE_PROTOCOL_H

#include <cstdint>

namespace can::protocol {
enum RPC : uint8_t {
  Restart = 0x03,
  NewModule = 0x05,
  SetFlowRate = 0x07,
  LastNode = 0x09
};

struct RestartCommand {};

struct NewModuleCommand {
  uint8_t numDosers;
  uint16_t maxFlowRate; // ml/min
};

struct NewModuleResponse {
  uint32_t moduleID;
};

struct LastNodeCommand {};

struct SetFlowRateCommand {
  uint32_t doserID;
  uint16_t flowRate; // ml/min
};

constexpr uint8_t responseID(RPC rpc) { return rpc + 1; }
} // namespace can::protocol

#endif // NUTRIDOSERSOFTWARE_PROTOCOL_H