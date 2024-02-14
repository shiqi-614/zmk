
/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zephyr/kernel.h>
#include <dt-bindings/zmk/mouse.h>

#define MOUSEKEY_MOVE_DELTA 5
#define MOUSEKEY_MAX_SPEED 10
#define MOUSEKEY_TIME_TO_MAX 50
#define MOUSEKEY_WHEEL_MAX_SPEED 8
#define MOUSEKEY_WHEEL_TIME_TO_MAX 40
#define MOUSEKEY_MOVE_MAX       127
#define MOUSEKEY_WHEEL_MAX      127
#define MOUSEKEY_WHEEL_DELTA    1


typedef uint16_t zmk_mouse_button_flags_t;
typedef uint16_t zmk_mouse_button_t;

struct mouse_config {
    int move_delta;
    int max_speed;
    int time_to_max;
};

struct vector2d {
    int x;
    int y;
};


struct k_work_q *zmk_mouse_work_q();
int zmk_mouse_init();
