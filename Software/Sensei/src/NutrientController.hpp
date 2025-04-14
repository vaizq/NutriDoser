#ifndef NUTRIENT_CONTROLLER_HPP
#define NUTRIENT_CONTROLLER_HPP

#include "DoserManager.hpp"
#include "Sensor.hpp"
#include <ArduinoJson.h>
#include <chrono>
#include <map>
#include <semaphore>
#include <thread>

using NutrientSchedule = std::map<int, float>;

class NutrientController {
  using Doser = DoserManager::Doser;

public:
  struct Config {
    float target;
    float acceptedError{};
    float flowRate{};
    Clock::duration adjustInterval{};
    NutrientSchedule schedule;
  };

  NutrientController(const Sensor &ecSensor) : ecSensor{ecSensor} {}

  void start(const Config &config) {
    this->config = config;
    dosers.clear();
    for (auto [id, _] : config.schedule) {
      auto doser = gDoserManager->lendDoser(id);
      if (!doser) {
        dosers.clear();
        throw std::logic_error("doser " + std::to_string(id) +
                               " is not available");
      }
      dosers.emplace(id, std::move(*doser));
    }

    running = true;
    sem.release();
  }

  void stop() {
    dosers.clear();
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
    if (const float ec = ecSensor.reading();
        ec + config.acceptedError < config.target) {
      std::vector<std::thread> doses;
      for (auto [id, amount] : config.schedule) {
        doses.emplace_back([this, id, amount]() {
          dose(dosers.at(id), amount, config.flowRate);
        });
      }

      for (auto &dose : doses) {
        dose.join();
      }
    }
  }

  const Sensor &ecSensor;
  Config config;
  std::map<int, Doser> dosers;
  std::binary_semaphore sem{0};
  bool running{false};
};

void convertToJson(const NutrientController::Config &config, JsonVariant doc) {
  for (const auto &[id, amount] : config.schedule) {
    doc["schedule"][std::to_string(id).c_str()].set(amount);
  }

  doc["target"].set(config.target);
  doc["acceptedErr"].set(config.acceptedError);
  doc["adjustmentInterval"].set(config.adjustInterval);
  doc["flowRate"].set(config.flowRate);
}

void convertFromJson(JsonVariantConst doc, NutrientController::Config &config) {
  auto schedule = doc["schedule"].as<JsonObjectConst>();
  for (JsonPairConst kv : schedule) {
    config.schedule[std::stoi(kv.key().c_str())] = kv.value();
  }

  config.target = doc["target"];
  config.acceptedError = doc["acceptedErr"];
  config.adjustInterval = doc["adjustmentInterval"];
  config.flowRate = doc["flowRate"];
}

#endif