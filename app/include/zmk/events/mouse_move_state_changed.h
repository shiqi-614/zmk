/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <zmk/event_manager.h>
#include <zmk/mouse.h>

struct zmk_mouse_move_state_changed {
    struct vector2d state;
    struct mouse_config config;
    bool pressed;
    int64_t timestamp;
};

ZMK_EVENT_DECLARE(zmk_mouse_move_state_changed);

static inline struct zmk_mouse_move_state_changed_event *
zmk_mouse_move_state_changed_from_encoded(uint32_t encoded, struct mouse_config config,
                                          bool pressed, int64_t timestamp) {
    struct vector2d state = (struct vector2d){
        .x = MOVE_HOR_DECODE(encoded),
        .y = MOVE_VERT_DECODE(encoded),
    };

    return new_zmk_mouse_move_state_changed((struct zmk_mouse_move_state_changed){
        .state = state, .config = config, .pressed = pressed, .timestamp = timestamp});
}

