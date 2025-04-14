#ifndef ANALOG_SENSOR_HPP
#define ANALOG_SENSOR_HPP 

#include <cstdint>
#include <utility>
#include <thread>
#include "util.h"
#include "nvs.h"
#include "esp_adc/adc_oneshot.h"
#include <deque>
#include "Sensor.hpp"


struct CalibrationPoint
{
    float value;
    float voltage;
};

// Linear analog sensor with two-point calibration
class AnalogSensor : public Sensor
{
public:
    using CalibrationData = std::pair<CalibrationPoint, CalibrationPoint>;

    AnalogSensor(int ioNum, CalibrationData calibration, const char* nvsNameSpace);
    void read();
    float reading() const override;
    void calibrate(float actual);
    void factoryReset();

private:
    struct Reading {
        float value;
        float voltage;
    };

    void storeCalibration(CalibrationData calib) const;
    std::optional<CalibrationData> loadCalibration() const;

    adc_channel_t channel;
    CalibrationData calibration;
    CalibrationData factoryCalibration;
    const char* nvsNameSpace;
    std::deque<Reading> readings;
    std::atomic<float> value;
    std::mutex mtx;
};



#endif