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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"


#include "communication/can.hpp"
#include "display/lcd.hpp"
#include "faces.h"
#include "servoControl.h"
#include "pinout.hpp"


#include "servo/ears.cpp"

extern "C" void app_main()
{
    esp_log_level_set("CAN", ESP_LOG_DEBUG);

    start_can();
    start_display();
    
    servoControl servo_r;
    // servoControl servo_l;
	// servo_l.attach(PIN_EAR_LEFT, 60, 2100);
    // servoDeg90();
	servo_r.attach(PIN_EAR_LEFT);
    vTaskDelay(500 / portTICK_RATE_MS);
    servo_r.write(180);
    servo_r.detach();
    
	servo_r.attach(PIN_EAR_LEFT);
    vTaskDelay(500 / portTICK_RATE_MS);
    
    //Go do nice stuff.
    // servo_r.write(180);
    // servo_l.write(180);
    send_image(dev_lcdSpi, BLINK_JPG);
    vTaskDelay(500 / portTICK_RATE_MS);
    servo_r.write(180);
    // servo_l.write(45);
    send_image(dev_lcdSpi, HAPPY_JPG);
    vTaskDelay(500 / portTICK_RATE_MS);
    
    servo_r.write(125);
    // servo_l.write(125);
    send_image(dev_lcdSpi, SAD_JPG);
    vTaskDelay(500 / portTICK_RATE_MS);
    
    servo_r.write(35);
    // servo_l.write(35);
    send_image(dev_lcdSpi, ANGRY_JPG);
    vTaskDelay(500 / portTICK_RATE_MS);
    send_image(dev_lcdSpi, BLINK_JPG);
    vTaskDelay(300 / portTICK_RATE_MS);
    send_image(dev_lcdSpi, CALM_JPG);
    servo_r.write(0);
    // servo_l.write(0);

    while (1) { vTaskDelay(1); }
}
