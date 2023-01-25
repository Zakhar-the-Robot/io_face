// *************************************************************************
//
// Copyright (c) 2023 Andrei Gramakov. All rights reserved.
//
// site:    https://agramakov.me
// e-mail:  mail@agramakov.me
//
// *************************************************************************

#include <math.h>
#include <sdkconfig.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <driver/gpio.h>
#include <driver/ledc.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "pinout.hpp"

class ServoMotor {
private:
    const float SERVO_MS_MIN = 0.06;
    const float SERVO_MS_MAX = 2.1;
    
    int m_pin;
    ledc_channel_t m_ledc_channel;

public:
    ServoMotor::ServoMotor(int pin);
    void setAngle(uint8_t deg);
}

ServoMotor::ServoMotor(int pin, ledc_channel_t ledc_channel)
    : m_pin(pin), m_ledc_channel(ledc_channel)
{
    // Timer configuration
    ledc_timer_config_t ledc_timer = { .speed_mode      = LEDC_LOW_SPEED_MODE,
                                       .duty_resolution = LEDC_TIMER_13_BIT,
                                       .timer_num       = LEDC_TIMER_0,
                                       .freq_hz         = 50,
                                       .clk_cfg         = LEDC_AUTO_CLK };
    ledc_timer_config(&ledc_timer);
    
    // LEDC configuration
    ledc_channel_config_t ledc_channel = { .gpio_num   = m_pin,
                                           .speed_mode = LEDC_LOW_SPEED_MODE,
                                           .channel    = m_ledc_channel,
                                           .intr_type  = LEDC_INTR_DISABLE,
                                           .timer_sel  = LEDC_TIMER_0,
                                           .duty       = 0,
                                           .hpoint     = 0 };
    ledc_channel_config(&ledc_channel);
};

void ServoMotor::setAngle(uint8_t deg) {
    int duty = (int) (100.0 * (SERVO_MS_MIN / 20.0) * 81.91);
    printf("%fms, duty = %f%% -> %d\n", SERVO_MS_MIN, 100.0 * (SERVO_MS_MIN / 20.0), duty);
    ledc_set_duty(LEDC_LOW_SPEED_MODE, LEDC_CHANNEL_0, duty);
}
