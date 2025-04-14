//
// Created by vaige on 20.7.2024.
//

#include "AnalogSensor.hpp"
#include "esp_adc/adc_continuous.h"
#include "adc.hpp"
#include "freertos/FreeRTOS.h"
#include <esp_log.h>
#include "util.h"


AnalogSensor::AnalogSensor(int ioNum, CalibrationData calibrationPoints, const char* nvsNameSpace)
    : calibration{calibrationPoints}, factoryCalibration{calibrationPoints}, nvsNameSpace{nvsNameSpace}
{
    if (calibrationPoints.first.voltage == calibrationPoints.second.voltage) {
        throw std::logic_error("calibration point voltages can't equal");
    }

    if (auto calib = loadCalibration(); calib) {
        ESP_LOGI(nvsNameSpace, "calibration loaded");
        calibration = *calib;
    } else {
        ESP_LOGI(nvsNameSpace, "using factory calibration");
    }

    adc_unit_t unitID;
    ESP_ERROR_CHECK(adc_continuous_io_to_channel(ioNum, &unitID, &channel));

    adc_oneshot_chan_cfg_t config = {
        .atten = ADC_ATTEN_DB_12,
        .bitwidth = ADC_BITWIDTH_12,
    };

    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc::unit, channel, &config));

    // Read once to make sure that the readings buffer is never empty
    vTaskDelay(pdMS_TO_TICKS(1));
    read();
}

void AnalogSensor::read()
{
    const float voltage = [](adc_channel_t channel) {
        constexpr int n = 10;
        int raw[n];
        for (int i = 0; i < n; ++i) {
            ESP_ERROR_CHECK(adc_oneshot_read(adc::unit, channel, &raw[i]));
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        long avg = std::accumulate(raw, raw + n, 0L) / n;
        return 3.3f * avg / 0xFFF;
    }(channel);

    std::lock_guard guard{mtx};

    CalibrationPoint& lowPoint = calibration.first;
    CalibrationPoint& highPoint = calibration.second;

    const float k = (highPoint.value - lowPoint.value) / (highPoint.voltage - lowPoint.voltage);
    readings.push_back({lowPoint.value + k * (voltage - lowPoint.voltage), voltage});

    if (readings.size() > 10) {
        readings.pop_front();
    }

    value = readings.back().value;
}

float AnalogSensor::reading() const
{
    return value;
}

/* 
    Calculates average voltage from previous readings,
    Then updates either low or high calibration-point.
*/
void AnalogSensor::calibrate(float value)
{
    CalibrationData localCalib;
    {
        std::lock_guard guard{mtx};

        const float voltage = [this]() {
            return std::accumulate(readings.begin(), readings.end(), 0.0f,
            [](float acc, const AnalogSensor::Reading& reading) {
                return acc + reading.voltage;
            }) / readings.size();
        }();

        CalibrationPoint& lowPoint = calibration.first;
        CalibrationPoint& highPoint = calibration.second;

        if (std::abs(lowPoint.value - value) < std::abs(highPoint.value - value))
        {
            lowPoint.value = value;
            lowPoint.voltage = voltage;
        }
        else
        {
            highPoint.value = value;
            highPoint.voltage = voltage;
        }

        localCalib = calibration;
    }

    storeCalibration(localCalib);
}

void AnalogSensor::factoryReset()
{
    CalibrationData localCalib;
    {
        std::lock_guard guard{mtx};
        calibration = factoryCalibration;
        localCalib = calibration;
    }
    storeCalibration(localCalib);
}

void AnalogSensor::storeCalibration(CalibrationData calib) const
{
    nvs_handle_t handle;

    if (auto err = nvs_open(nvsNameSpace, NVS_READWRITE, &handle); err != ESP_OK)
        throw std::runtime_error(esp_err_to_name(err));

    ez::ScopeGuard closeGuard([handle]() {
        nvs_close(handle);
    });

    if (auto err = nvs_set_blob(handle, "lowPoint", &calib.first, sizeof(calib.first)); err != ESP_OK)
        throw std::runtime_error(esp_err_to_name(err));

    if (auto err = nvs_set_blob(handle, "highPoint", &calib.second, sizeof(calib.second)); err != ESP_OK)
        throw std::runtime_error(esp_err_to_name(err));

    if (auto err = nvs_commit(handle); err != ESP_OK)
        throw std::runtime_error(esp_err_to_name(err));
}

std::optional<AnalogSensor::CalibrationData> AnalogSensor::loadCalibration() const
{
    CalibrationData calib;
    nvs_handle_t handle;

    if (auto err = nvs_open(nvsNameSpace, NVS_READWRITE, &handle); err != ESP_OK)
        throw std::runtime_error(esp_err_to_name(err));

    ez::ScopeGuard closeGuard([handle]() {
        nvs_close(handle);
    });

    std::size_t length = sizeof(calib.first);
    if (auto err = nvs_get_blob(handle, "lowPoint", &calib.first, &length); err != ESP_OK)
        return std::nullopt;

    if (auto err = nvs_get_blob(handle, "highPoint", &calib.second, &length); err != ESP_OK) 
        return std::nullopt;

    return calib;
}