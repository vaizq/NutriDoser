#include "i2c.hpp"

i2c_master_bus_handle_t i2c::bus;

esp_err_t i2c::init(gpio_num_t sdaio, gpio_num_t sclio) {
  i2c_master_bus_config_t i2c_mst_config = {.i2c_port = -1,
                                            .sda_io_num = sdaio,
                                            .scl_io_num = sclio,
                                            .clk_source = I2C_CLK_SRC_DEFAULT,
                                            .glitch_ignore_cnt = 7,
                                            .flags{
                                                .enable_internal_pullup = true,
                                            }};

  return i2c_new_master_bus(&i2c_mst_config, &bus);
}

esp_err_t i2c::shutdown() { return i2c_del_master_bus(bus); }