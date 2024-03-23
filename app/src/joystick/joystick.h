/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <nrfx_saadc.h>

struct io_channel_config {
    const char *label;
    uint8_t channel;
};

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

void send_joystick_report(struct joystick_data data);

void get_joystick_report(int adcX, int adcY, struct joystick_data *data);
