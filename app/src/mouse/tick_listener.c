/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/mouse_tick.h>
#include <zmk/endpoints.h>
#include <zmk/mouse.h>
#include <zmk/hid.h>

static uint8_t move_unit(int mousekey_repeat, struct mouse_config config) {
    uint16_t unit;
    if (mousekey_repeat == 0) {
        unit = config.move_delta;
    } else if (mousekey_repeat >= config.time_to_max) {
        unit = config.move_delta * config.max_speed;
    } else {
        unit = (config.move_delta * config.max_speed * mousekey_repeat) / config.time_to_max;
    }
    return (unit > MOUSEKEY_MOVE_MAX ? MOUSEKEY_MOVE_MAX : (unit == 0 ? 1 : unit));
}

static uint8_t wheel_unit(int mousekey_repeat, struct mouse_config config) {
    uint16_t unit;
    if (mousekey_repeat == 0) {
        unit = config.move_delta;
    } else if (mousekey_repeat >= config.time_to_max) {
        unit = config.move_delta * config.max_speed;
    } else {
        unit = (config.move_delta * config.max_speed * mousekey_repeat) / config.time_to_max;
    }
    return (unit > MOUSEKEY_WHEEL_MAX ? MOUSEKEY_WHEEL_MAX : (unit == 0 ? 1 : unit));
}

static void mouse_tick_handler(const struct zmk_mouse_tick *tick) {
    LOG_DBG("enter mouse_tick_handler");

    int mousekey_repeat = tick->mousekey_repeat;
    struct vector2d move_state = {0};
    struct vector2d scroll_state = {0};

    if (tick->move_state.x > 0) {
        move_state.x = move_unit(mousekey_repeat, tick->move_config);
        LOG_DBG("move x unit %d", move_state.x);
    }
    if (tick->move_state.x < 0) {
        move_state.x = move_unit(mousekey_repeat, tick->move_config) * -1;
        LOG_DBG("move x unit reverse %d", move_state.x);
    }
    if (tick->move_state.y > 0) {
        move_state.y = move_unit(mousekey_repeat, tick->move_config);
        LOG_DBG("move y unit %d", move_state.y);
    }
    if (tick->move_state.y < 0) {
        move_state.y = move_unit(mousekey_repeat, tick->move_config) * -1;
        LOG_DBG("move y unit reverse %d", move_state.y);
    }

    /* diagonal move [1/sqrt(2) = 0.7] */
    if (move_state.x && move_state.y) {
        move_state.x *= 0.7;
        move_state.y *= 0.7;
    }

    if (tick->scroll_state.x > 0)
        scroll_state.x = wheel_unit(mousekey_repeat, tick->scroll_config);
    if (tick->scroll_state.x < 0)
        scroll_state.x = wheel_unit(mousekey_repeat, tick->scroll_config) * -1;
    if (tick->scroll_state.y > 0)
        scroll_state.y = wheel_unit(mousekey_repeat, tick->scroll_config);
    if (tick->scroll_state.y < 0)
        scroll_state.y = wheel_unit(mousekey_repeat, tick->scroll_config) * -1;

    LOG_DBG("mousekey_repeat %d", mousekey_repeat);
    LOG_DBG("move unit %d %d", move_state.x, move_state.y);
    LOG_DBG("scroll unit %d %d", scroll_state.x, scroll_state.y);

    zmk_hid_mouse_movement_set(move_state.x, move_state.y);
    zmk_hid_mouse_scroll_set(scroll_state.x, scroll_state.y);
    zmk_endpoints_send_mouse_report();
}

int zmk_mouse_tick_listener(const zmk_event_t *eh) {
    const struct zmk_mouse_tick *tick = as_zmk_mouse_tick(eh);
    if (tick) {
        mouse_tick_handler(tick);
        return 0;
    }
    return 0;
}

ZMK_LISTENER(zmk_mouse_tick_listener, zmk_mouse_tick_listener);
ZMK_SUBSCRIPTION(zmk_mouse_tick_listener, zmk_mouse_tick);
