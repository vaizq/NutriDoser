#ifndef I2C_HPP
#define I2C_HPP

#include "driver/i2c_master.h"

namespace i2c {

extern i2c_master_bus_handle_t bus;

esp_err_t init(gpio_num_t sdaio, gpio_num_t sclio);
esp_err_t shutdown();

} // namespace i2c

#endif