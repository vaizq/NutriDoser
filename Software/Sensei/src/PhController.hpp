#ifndef PH_CONTROLLER_HPP
#define PH_CONTROLLER_HPP

#include "DoserManager.hpp"
#include "Sensor.hpp"
#include <ArduinoJson.h>
#include <chrono>
#include <map>
#include <semaphore>
#include <thread>

class PhController {
  using Doser = DoserManager::Doser;

public:
  struct Config {
    float target;
    float acceptedError{};
    float flowRate{};
    float doseAmount{};
    Clock::duration adjustInterval{};
    std::optional<int> pHDownDoser;
    std::optional<int> pHUpDoser;
  };

  PhController(const Sensor &phSensor) : phSensor{phSensor} {}

  void start(const Config &config) {
    this->config = config;

    if (config.pHDownDoser) {
      const int id = *config.pHDownDoser;
      auto doser = gDoserManager->lendDoser(id);
      if (!doser) {
        phDownDoser.reset();
        phUpDoser.reset();
        throw std::logic_error("doser " + std::to_string(id) +
                               " is not available");
      }
      phDownDoser = std::move(doser);
    }

    if (config.pHUpDoser) {
      const int id = *config.pHUpDoser;
      auto doser = gDoserManager->lendDoser(id);
      if (!doser) {
        phDownDoser.reset();
        phUpDoser.reset();
        throw std::logic_error("doser " + std::to_string(id) +
                               " is not available");
      }
      phUpDoser = std::move(doser);
    }

    running = true;
    sem.release();
  }

  void stop() {
    phDownDoser.reset();
    phUpDoser.reset();
    running = false;
  }

  void run() {
    for (;;) {
      if (running)
        adjust();
      sem.try_acquire_for(config.adjustInterval);
    }
  }

  bool isRunning() const { return running; }

private:
  void adjust() {
    const float ph = phSensor.reading();
    const float err = config.target - ph;

    if (err < config.acceptedError && phDownDoser) {
      dose(*phDownDoser, config.doseAmount, config.flowRate);
    } else if (err > config.acceptedError && phUpDoser) {
      dose(*phUpDoser, config.doseAmount, config.flowRate);
    }
  }

  const Sensor &phSensor;
  Config config;
  std::optional<Doser> phDownDoser;
  std::optional<Doser> phUpDoser;
  std::binary_semaphore sem{0};
  bool running{false};
};

void convertToJson(const PhController::Config &config, JsonVariant doc) {
  doc["doseAmount"] = config.doseAmount;
  if (config.pHDownDoser) {
    doc["pHDownDoser"].set(*config.pHDownDoser);
  }
  if (config.pHUpDoser) {
    doc["pHUpDoser"].set(*config.pHUpDoser);
  }

  doc["target"].set(config.target);
  doc["acceptedError"].set(config.acceptedError);
  doc["adjustInterval"].set(
      std::chrono::duration_cast<std::chrono::duration<double>>(
          config.adjustInterval)
          .count());
  doc["flowRate"] = config.flowRate;
}

void convertFromJson(JsonVariantConst doc, PhController::Config &config) {
  if (doc.containsKey("pHDownDoser") && doc["pHDownDoser"] >= 0) {
    config.pHDownDoser = doc["pHDownDoser"];
  }
  if (doc.containsKey("pHUpDoser") && doc["pHUpDoser"] >= 0) {
    config.pHUpDoser = doc["pHUpDoser"];
  }

  config.doseAmount = doc["doseAmount"];
  config.target = doc["target"];
  config.acceptedError = doc["acceptedError"];
  config.adjustInterval = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(doc["adjustInterval"].as<double>()));
  config.flowRate = doc["flowRate"];
}

#endif