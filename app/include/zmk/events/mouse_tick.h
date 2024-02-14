
/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <dt-bindings/zmk/mouse.h>
#include <zmk/event_manager.h>
#include <zmk/mouse.h>

struct zmk_mouse_tick {
    struct vector2d move_state;
    struct vector2d scroll_state;
    struct mouse_config move_config;
    struct mouse_config scroll_config;
    int mousekey_repeat;
};

ZMK_EVENT_DECLARE(zmk_mouse_tick);

static inline struct zmk_mouse_tick_event *zmk_mouse_tick(struct vector2d move_state,
                                                          struct vector2d scroll_state,
                                                          struct mouse_config move_config,
                                                          struct mouse_config scroll_config,
                                                          int mousekey_repeat) {
    return new_zmk_mouse_tick((struct zmk_mouse_tick){
        .move_state = move_state,
        .scroll_state = scroll_state,
        .move_config = move_config,
        .scroll_config = scroll_config,
        .mousekey_repeat = mousekey_repeat,
    });
}


