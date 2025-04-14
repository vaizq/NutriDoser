#include "App.hpp"
#include "mqtt.hpp"
#include <ArduinoJson.h>
#include <cstdio>

std::unique_ptr<App> gApp;

void apiRun() {
  ez::mqtt::Client client{"mqtt://5.61.89.44:1883"};

  client.subscribe("sensei/doser/on", [](const JsonDocument &doc) {
    const int id = doc["doserID"];
    if (auto doser = gDoserManager->lendDoser(id); doser) {
      if (doser->tryOn(doc["flowRate"]))
        gApp->runningDosers.push_back(std::move(*doser));
    }
  });

  client.subscribe("sensei/doser/off", [](const JsonDocument &doc) {
    std::erase_if(gApp->runningDosers,
                  [id = doc["doserID"]](const auto &doser) {
                    return doser.getId() == id;
                  });
  });

  client.subscribe("sensei/doserManager/reset",
                   []() { gApp->runningDosers.clear(); });

  client.subscribe("sensei/pHSensor/calibrate", [](const JsonDocument &doc) {
    gApp->pHSensor->calibrate(doc["target"]);
  });

  client.subscribe("sensei/ecSensor/calibrate", [](const JsonDocument &doc) {
    gApp->ecSensor->calibrate(doc["target"]);
  });

  client.subscribe("sensei/pHSensor/factoryReset",
                   []() { gApp->pHSensor->factoryReset(); });

  client.subscribe("sensei/ecSensor/factoryReset",
                   []() { gApp->ecSensor->factoryReset(); });

  client.subscribe("sensei/pHController/start", [](const JsonDocument &doc) {
    auto config = doc["config"].as<PhController::Config>();
    gApp->pHController->start(config);
  });

  client.subscribe(
      "sensei/nutrientController/start", [](const JsonDocument &doc) {
        auto config = doc["config"].as<NutrientController::Config>();
        gApp->nutrientController->start(config);
      });

  client.subscribe("sensei/pHController/stop",
                   []() { gApp->pHController->stop(); });

  client.subscribe("sensei/nutrientController/stop",
                   []() { gApp->nutrientController->stop(); });

  client.start();

  for (char buf[1024];;) {
    const App::Status status = gApp->status();

    JsonDocument doc;
    convertToJson(status, doc);

    size_t n = serializeJson(doc, buf);
    client.publish("sensei/status", {buf, n});

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}

extern "C" void app_main(void) {
  gApp = std::make_unique<App>();
  apiRun();
}