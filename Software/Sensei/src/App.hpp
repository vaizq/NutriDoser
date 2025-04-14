#ifndef APP_HPP
#define APP_HPP

#include "AnalogSensor.hpp"
#include "CANDoserManager.hpp"
#include "Clock.hpp"
#include "DFRobot_RGBLCD1602.h"
#include "DeltaTimer.hpp"
#include "NutrientController.hpp"
#include "PhController.hpp"
#include "adc.hpp"
#include "can.h"
#include "wifi.hpp"
#include <memory>
#include <thread>

// Goal: provide a public thread-safe api for server to use

class App {
public:
  enum class State { Init, Normal };

  struct Status {
    float ph;
    float ec;
    std::vector<float> flowRates;
    bool pHControllerRunning;
    bool nutrientContollerRunning;
  };

  App() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_ERROR_CHECK(i2c::init(GPIO_NUM_5, GPIO_NUM_18));
    ESP_ERROR_CHECK(adc::init());
    ESP_ERROR_CHECK(can::init());

    lcd = std::make_unique<DFRobot_RGBLCD1602>(0x60);

    uiThread = std::jthread([this]() {
      lcd->init();
      for (;;) {
        switch (state) {
        case State::Init:
          uiInitializing();
          break;
        case State::Normal:
          uiNormal();
          break;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
      }
    });

    wifi_init();

    pHSensor = std::make_unique<AnalogSensor>(
        GPIO_NUM_36,
        AnalogSensor::CalibrationData{CalibrationPoint{4.1f, 2.1f},
                                      CalibrationPoint{7.f, 1.5f}},
        "PH_sensor");
    ecSensor = std::make_unique<AnalogSensor>(
        GPIO_NUM_39,
        AnalogSensor::CalibrationData{CalibrationPoint{0.f, 0.f},
                                      CalibrationPoint{3.f, 3.f}},
        "EC_sensor");

    gDoserManager = std::make_unique<CANDoserManager>(1);

    nutrientController = std::make_unique<NutrientController>(*ecSensor);

    pHController = std::make_unique<PhController>(*pHSensor);

    sensorThread = std::jthread([this]() {
      for (;;) {
        pHSensor->read();
        vTaskDelay(pdMS_TO_TICKS(250));
        ecSensor->read();
        vTaskDelay(pdMS_TO_TICKS(250));
      }
    });

    nutrientControllerThread =
        std::jthread([this]() { nutrientController->run(); });

    pHControllerThread = std::jthread([this]() { pHController->run(); });

    state = State::Normal;
  }

  ~App() {
    ESP_ERROR_CHECK(i2c::shutdown());
    ESP_ERROR_CHECK(adc::shutdown());
    ESP_ERROR_CHECK(can::shutdown());
  }

  Status status() const {
    return {pHSensor->reading(), ecSensor->reading(),
            gDoserManager->getFlowRates(), pHController->isRunning(),
            nutrientController->isRunning()};
  }

  std::unique_ptr<AnalogSensor> pHSensor;
  std::unique_ptr<AnalogSensor> ecSensor;
  std::unique_ptr<NutrientController> nutrientController;
  std::unique_ptr<PhController> pHController;
  std::vector<DoserManager::Doser> runningDosers;

private:
  void uiInitializing() {
    static int nDots = 0;
    std::string msg = "Loading";
    for (int i = 0; i < nDots; ++i) {
      msg += '.';
    }
    nDots = (nDots + 1) % 4;

    lcd->clear();
    lcd->print(msg.c_str());
  }

  void uiNormal() {
    lcd->clear();
    lcd->print("PH: %.2f", pHSensor->reading());
    lcd->setCursor(0, 1);
    lcd->print("EC: %.2f", ecSensor->reading());
  }

  std::unique_ptr<DFRobot_RGBLCD1602> lcd;
  State state{State::Init};
  std::jthread sensorThread;
  std::jthread uiThread;
  std::jthread dosingThread;
  std::jthread nutrientControllerThread;
  std::jthread pHControllerThread;
};

extern std::unique_ptr<App> gApp;

inline void convertToJson(const App::Status &status, JsonVariant doc) {
  doc["ph"] = status.ph;
  doc["ec"] = status.ec;

  JsonArray dosers = doc.createNestedArray("dosers");
  for (auto &flowRate : status.flowRates) {
    dosers.createNestedObject()["maxFlowRate"] = flowRate;
  }

  doc["pHControllerRunning"] = status.pHControllerRunning;
  doc["nutrientControllerRunning"] = status.nutrientContollerRunning;
}

inline void convertFromJson(JsonVariantConst doc, App::Status &status) {
  status.ph = doc["ph"].as<float>();
  status.ec = doc["ec"].as<float>();

  status.flowRates.clear();
  if (doc["dosers"].is<JsonArrayConst>()) {
    for (JsonVariantConst doserJson : doc["dosers"].as<JsonArrayConst>()) {
      auto flowRate = doserJson["maxFlowRate"].as<float>();
      status.flowRates.push_back(flowRate);
    }
  }

  status.pHControllerRunning = doc["pHControllerStatus"];
  status.nutrientContollerRunning = doc["nutrientControllerStatus"];
}

#endif