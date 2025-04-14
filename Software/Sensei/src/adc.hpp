#ifndef ADC_HPP
#define ADC_HPP

#include "esp_adc/adc_oneshot.h"

namespace adc {

extern adc_oneshot_unit_handle_t unit;

esp_err_t init();
esp_err_t shutdown();

} // namespace adc

#endif