/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <nrfx_saadc.h>

#define NUM_ADC_CHANS 3

static const nrf_saadc_input_t ANALOG_INPUT_MAP[] = {
    NRF_SAADC_INPUT_AIN0, NRF_SAADC_INPUT_AIN1, NRF_SAADC_INPUT_AIN2, NRF_SAADC_INPUT_AIN3,
    NRF_SAADC_INPUT_AIN4, NRF_SAADC_INPUT_AIN5, NRF_SAADC_INPUT_AIN6, NRF_SAADC_INPUT_AIN7};


struct joystick_config {
    int x_channel_idx;
    int y_channel_idx;
    nrf_saadc_reference_t adc_reference;
    nrf_saadc_gain_t adc_gain;
    int polling_rate_hertz;
    int move_step;
    float deadzone;

};

struct joystick_data {
    int raw_x;
    int raw_y;
    int x;
    int y;
    struct k_work work;
    struct joystick_config config;
};

static struct joystick_data joy_data;

void send_joystick_report(struct joystick_data *data);

void get_joystick_report(int adcX, int adcY, struct joystick_data *data);
