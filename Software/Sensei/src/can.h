//
// Created by vaige on 15.7.2024.
//

#ifndef UNTITLED3_CAN_H
#define UNTITLED3_CAN_H

#include "driver/twai.h"

namespace can
{

#define TWAI_TX_PIN GPIO_NUM_21
#define TWAI_RX_PIN GPIO_NUM_22

    esp_err_t init()
    {
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(TWAI_TX_PIN, TWAI_RX_PIN, TWAI_MODE_NORMAL);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_50KBITS();
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        if (esp_err_t err = twai_driver_install(&g_config, &t_config, &f_config); err != ESP_OK)
            return err;

        return twai_start();
    }

    esp_err_t shutdown()
    {
        if (esp_err_t err = twai_stop(); err != ESP_OK)
            return err;

        return twai_driver_uninstall();
    }
}


#endif //UNTITLED3_CAN_H
