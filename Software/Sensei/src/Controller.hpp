#ifndef CONTROLLER_HPP
#define CONTROLLER_HPP

#include "Sensor.hpp"
#include "util.h"
#include <ArduinoJson.h>
#include <cassert>
#include <chrono>
#include <functional>
#include <future>
#include <map>
#include <numeric>
#include <semaphore>
#include <thread>

using Doser = int;
using NutrientSchedule = std::map<Doser, float>; // Doser -> ml/L
using DoseMethod =
    std::function<std::future<void>(Doser doser, float amount, float flowRate)>;

struct Config {
  float target;
  float acceptedErr;
  Clock::duration adjustmentInterval;
  float flowRate;
};

template <typename Derived, typename ConfigType = Config> class Controller {
public:
  enum class Status { On, Off };

  static std::string to_string(Status status) {
    return status == Status::On ? "On" : "Off";
  }

  static Status from_string(const std::string &s) {
    return s == "On" ? Status::On : Status::Off;
  }

  Status status() const { return mStatus; }

  const ConfigType &config() const { return mConfig; }

  void start(const ConfigType &config) {
    mConfig = config;
    restart();
  }

  void stop() { mStatus = Status::Off; }

  void restart() {
    mStatus = Status::On;
    mSmph.release();
  }

  void run() {
    for (;;) {
      if (mStatus == Status::On) {
        static_cast<Derived *>(this)->update();
      }
      mSmph.try_acquire_for(mConfig.adjustmentInterval);
    }
  }

protected:
  Controller() = default;
  Status mStatus{Status::Off};
  ConfigType mConfig;

private:
  std::binary_semaphore mSmph{0};
};

struct NutrientControllerConfig : public Config {
  NutrientSchedule schedule;
};

class NutrientController
    : public Controller<NutrientController, NutrientControllerConfig> {
public:
  NutrientController(const Sensor &ecSensor, DoseMethod doseMethod)
      : ecSensor{ecSensor}, doseMethod{doseMethod} {}

  void update() {
    const float ec = ecSensor.reading();
    const float err = mConfig.target - ec;

    if (err > mConfig.acceptedErr) {
      std::vector<std::future<void>> doses;
      for (auto [doser, amount] : mConfig.schedule) {
        doses.push_back(doseMethod(doser, amount, mConfig.flowRate));
      }
      for (auto &dose : doses) {
        dose.wait();
      }
    }
  }

private:
  const Sensor &ecSensor;
  DoseMethod doseMethod;
};

struct PHControllerConfig : public Config {
  float doseAmount;
  std::optional<Doser> pHDownDoser;
  std::optional<Doser> pHUpDoser;
};

class PHController : public Controller<PHController, PHControllerConfig> {
public:
  PHController(const Sensor &phSensor, DoseMethod doseMethod)
      : phSensor{phSensor}, doseMethod{doseMethod} {}

  void update() {
    const float ph = phSensor.reading();
    const float err = mConfig.target - ph;

    if (err < mConfig.acceptedErr && mConfig.pHDownDoser) {
      doseMethod(*mConfig.pHDownDoser, mConfig.doseAmount, mConfig.flowRate)
          .wait();
    } else if (err > mConfig.acceptedErr && mConfig.pHUpDoser) {
      doseMethod(*mConfig.pHUpDoser, mConfig.doseAmount, mConfig.flowRate)
          .wait();
    }
  }

private:
  const Sensor &phSensor;
  DoseMethod doseMethod;
};

void convertToJson(const NutrientControllerConfig &config, JsonVariant doc) {
  for (const auto &[doser, amount] : config.schedule) {
    doc["schedule"][std::to_string(doser).c_str()].set(amount);
  }

  doc["target"].set(config.target);
  doc["acceptedErr"].set(config.acceptedErr);
  doc["adjustmentInterval"].set(config.adjustmentInterval);
  doc["flowRate"].set(config.flowRate);
}

void convertFromJson(JsonVariantConst doc, NutrientControllerConfig &config) {
  auto schedule = doc["schedule"].as<JsonObjectConst>();
  for (JsonPairConst kv : schedule) {
    config.schedule[std::stoi(kv.key().c_str())] = kv.value();
  }

  config.target = doc["target"];
  config.acceptedErr = doc["acceptedErr"];
  config.adjustmentInterval = doc["adjustmentInterval"];
  config.flowRate = doc["flowRate"];
}

void convertToJson(const PHControllerConfig &config, JsonVariant doc) {
  doc["doseAmount"] = config.doseAmount;
  if (config.pHDownDoser) {
    doc["pHDownDoser"].set(*config.pHDownDoser);
  }
  if (config.pHUpDoser) {
    doc["pHUpDoser"].set(*config.pHUpDoser);
  }

  doc["target"].set(config.target);
  doc["acceptedErr"].set(config.acceptedErr);
  doc["adjustmentInterval"].set(
      std::chrono::duration_cast<std::chrono::duration<double>>(
          config.adjustmentInterval)
          .count());
  doc["flowRate"] = config.flowRate;
}

void convertFromJson(JsonVariantConst doc, PHControllerConfig &config) {
  if (doc.containsKey("pHDownDoser") && doc["pHDownDoser"] >= 0) {
    config.pHDownDoser = doc["pHDownDoser"];
  }
  if (doc.containsKey("pHUpDoser") && doc["pHUpDoser"] >= 0) {
    config.pHUpDoser = doc["pHUpDoser"];
  }

  config.doseAmount = doc["doseAmount"];
  config.target = doc["target"];
  config.acceptedErr = doc["acceptedErr"];
  config.adjustmentInterval = std::chrono::duration_cast<Clock::duration>(
      std::chrono::duration<double>(doc["adjustmentInterval"].as<double>()));
  config.flowRate = doc["flowRate"];
}

#endif