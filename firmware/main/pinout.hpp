// *************************************************************************
//
// Copyright (c) 2022 Andrei Gramakov. All rights reserved.
//
// This file is licensed under the terms of the MIT license.
// For a copy, see: https://opensource.org/licenses/MIT
//
// site:    https://agramakov.me
// e-mail:  mail@agramakov.me
//
// *************************************************************************

#pragma once

#include "driver/gpio.h"
#include "esp_system.h"

/* Servo Ears*/
#define PIN_EAR_LEFT    GPIO_NUM_26
#define PIN_EAR_RIGHT   GPIO_NUM_12

/* CAN */
#define PIN_CANBUS_TX_ON_MODULE GPIO_NUM_27  // WARM/LIGHT COLOR and GREEN
#define PIN_CANBUS_RX_ON_MODULE GPIO_NUM_14  // COLD/DARK COLOR (e.g. BLUE)

/* Display */
#ifdef CONFIG_IDF_TARGET_ESP32

#define LCD_HOST HSPI_HOST
#define PIN_NUM_MISO static_cast<gpio_num_t>(25)
#define PIN_NUM_MOSI static_cast<gpio_num_t>(23)
#define PIN_NUM_CLK static_cast<gpio_num_t>(19)
#define PIN_NUM_CS static_cast<gpio_num_t>(22)
#define PIN_NUM_DC static_cast<gpio_num_t>(21)
#define PIN_NUM_RST static_cast<gpio_num_t>(18)
#define PIN_NUM_BCKL static_cast<gpio_num_t>(5)

#elif defined CONFIG_IDF_TARGET_ESP32S2

#define LCD_HOST SPI2_HOST
#define PIN_NUM_MISO static_cast<gpio_num_t>(37)
#define PIN_NUM_MOSI static_cast<gpio_num_t>(35)
#define PIN_NUM_CLK static_cast<gpio_num_t>(36)
#define PIN_NUM_CS static_cast<gpio_num_t>(34)
#define PIN_NUM_DC static_cast<gpio_num_t>(4)
#define PIN_NUM_RST static_cast<gpio_num_t>(5)
#define PIN_NUM_BCKL static_cast<gpio_num_t>(6)

#elif defined CONFIG_IDF_TARGET_ESP32C3

#define LCD_HOST SPI2_HOST
#define PIN_NUM_MISO static_cast<gpio_num_t>(2)
#define PIN_NUM_MOSI static_cast<gpio_num_t>(7)
#define PIN_NUM_CLK static_cast<gpio_num_t>(6)
#define PIN_NUM_CS static_cast<gpio_num_t>(10)
#define PIN_NUM_DC static_cast<gpio_num_t>(9)
#define PIN_NUM_RST static_cast<gpio_num_t>(4)
#define PIN_NUM_BCKL static_cast<gpio_num_t>(5)

#endif
