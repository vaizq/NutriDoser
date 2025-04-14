#include "adc.hpp"

adc_oneshot_unit_handle_t adc::unit;

esp_err_t adc::init() {
  adc_oneshot_unit_init_cfg_t unitConfig = {
      .unit_id = ADC_UNIT_1,
      .ulp_mode = ADC_ULP_MODE_DISABLE,
  };

  return adc_oneshot_new_unit(&unitConfig, &unit);
}

esp_err_t adc::shutdown() { return adc_oneshot_del_unit(unit); }