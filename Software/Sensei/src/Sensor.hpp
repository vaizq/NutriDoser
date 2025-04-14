#ifndef SENSOR_HPP
#define SENSOR_HPP


struct Sensor {
    virtual float reading() const = 0;
};


#endif