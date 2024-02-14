/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>

struct io_channel_config {
    const char *label;
    uint8_t channel;
};

struct joy_config {
    int min_on;
    int frequency;
    bool reverse;
};

struct joy_data {
    int setup;

    uint16_t x_adc_raw;

    uint16_t y_adc_raw;

    int zero_value;
    int value;
    int delta;
    int last_rotate;
    int last_press;

    const struct device *dev;
};

void handle_joystick_report(int x, int y);
