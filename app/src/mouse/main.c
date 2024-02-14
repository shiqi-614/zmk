/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/kernel.h>
#include <zmk/mouse.h>

struct k_work_q *zmk_mouse_work_q() { return &k_sys_work_q; }

int zmk_mouse_init() { return 0; }
