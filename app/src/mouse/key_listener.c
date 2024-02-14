/*
 * Copyright (c) 2021 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#include <drivers/behavior.h>
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/events/mouse_button_state_changed.h>
#include <zmk/events/mouse_move_state_changed.h>
#include <zmk/events/mouse_scroll_state_changed.h>
#include <zmk/events/mouse_tick.h>
#include <zmk/hid.h>
#include <zmk/endpoints.h>
#include <zmk/mouse.h>

static struct vector2d move_state = {0};
static struct vector2d scroll_state = {0};
static struct mouse_config move_config = (struct mouse_config){0};
static struct mouse_config scroll_config = (struct mouse_config){0};
static bool tick_timer_running = false;
static int mousekey_repeat = 0;

bool equals(const struct mouse_config *one, const struct mouse_config *other) {
    return one->move_delta == other->move_delta && one->max_speed == other->max_speed &&
           one->time_to_max == other->time_to_max;
}

void mouse_timer_cb(struct k_timer *dummy) {
    if (mousekey_repeat < UINT8_MAX) {
        mousekey_repeat++;
    }
    ZMK_EVENT_RAISE(
        zmk_mouse_tick(move_state, scroll_state, move_config, scroll_config, mousekey_repeat));
    LOG_DBG("Submitting mouse work to queue");
}

void mouse_clear_cb(struct k_timer *dummy) {
    move_state = (struct vector2d){0};
    scroll_state = (struct vector2d){0};
    mousekey_repeat = 0;
    zmk_hid_mouse_clear();
    LOG_DBG("Clearing state");
}

K_TIMER_DEFINE(mouse_timer, mouse_timer_cb, mouse_clear_cb);

static void startTimeerIfNeeded() {
    if (move_state.x != 0 || move_state.y != 0 || scroll_state.x != 0 || scroll_state.y != 0) {
        if (tick_timer_running == false) {
            tick_timer_running = true;
            LOG_DBG("start mouse tick timer");
            k_timer_start(&mouse_timer, K_NO_WAIT, K_MSEC(50));
        }
    }
}

static void stopTimeerIfNeeded() {
    if (move_state.x == 0 && move_state.y == 0 && scroll_state.x == 0 && scroll_state.y == 0) {
        if (tick_timer_running == true) {
            tick_timer_running = false;
            mousekey_repeat = 0;
            zmk_hid_mouse_clear();
            LOG_DBG("stop mouse tick timer");
            k_timer_stop(&mouse_timer);
        }
    }
}

static void listener_mouse_move_pressed(const struct zmk_mouse_move_state_changed *ev) {
    move_state.x += ev->state.x;
    move_state.y += ev->state.y;
    LOG_DBG("mouse press x %d, y %d", move_state.x, move_state.y);
    LOG_DBG("mousekey_repeat is %d", mousekey_repeat);
    startTimeerIfNeeded();
}

static void listener_mouse_move_released(const struct zmk_mouse_move_state_changed *ev) {
    if (ev->state.x != 0) {
        move_state.x = 0;
    }
    if (ev->state.y != 0) {
        move_state.y = 0;
    }
    LOG_DBG("mousekey_repeat is %d", mousekey_repeat);
    stopTimeerIfNeeded();
}

static void listener_mouse_scroll_pressed(const struct zmk_mouse_scroll_state_changed *ev) {
    scroll_state.x += ev->state.x;
    scroll_state.y += ev->state.y;
    startTimeerIfNeeded();
}

static void listener_mouse_scroll_released(const struct zmk_mouse_scroll_state_changed *ev) {
    if (ev->state.x != 0) {
        scroll_state.x = 0;
    }
    if (ev->state.y != 0) {
        scroll_state.y = 0;
    }
    stopTimeerIfNeeded();
}

static void listener_mouse_button_pressed(const struct zmk_mouse_button_state_changed *ev) {
    LOG_DBG("buttons: 0x%02X", ev->buttons);
    zmk_hid_mouse_buttons_press(ev->buttons);
    zmk_endpoints_send_mouse_report();
}

static void listener_mouse_button_released(const struct zmk_mouse_button_state_changed *ev) {
    LOG_DBG("buttons: 0x%02X", ev->buttons);
    zmk_hid_mouse_buttons_release(ev->buttons);
    zmk_endpoints_send_mouse_report();
}

int mouse_listener(const zmk_event_t *eh) {
    const struct zmk_mouse_move_state_changed *mmv_ev = as_zmk_mouse_move_state_changed(eh);
    if (mmv_ev) {
        if (!equals(&move_config, &(mmv_ev->config)))
            move_config = mmv_ev->config;

        if (mmv_ev->pressed) {
            listener_mouse_move_pressed(mmv_ev);
        } else {
            listener_mouse_move_released(mmv_ev);
        }
        return 0;
    }
    const struct zmk_mouse_scroll_state_changed *msc_ev = as_zmk_mouse_scroll_state_changed(eh);
    if (msc_ev) {
        if (!equals(&scroll_config, &(msc_ev->config)))
            scroll_config = msc_ev->config;
        if (msc_ev->pressed) {
            listener_mouse_scroll_pressed(msc_ev);
        } else {
            listener_mouse_scroll_released(msc_ev);
        }
        return 0;
    }
    const struct zmk_mouse_button_state_changed *mbt_ev = as_zmk_mouse_button_state_changed(eh);
    if (mbt_ev) {
        if (mbt_ev->pressed) {
            listener_mouse_button_pressed(mbt_ev);
        } else {
            listener_mouse_button_released(mbt_ev);
        }
        return 0;
    }
    return 0;
}

ZMK_LISTENER(mouse_listener, mouse_listener);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_button_state_changed);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_move_state_changed);
ZMK_SUBSCRIPTION(mouse_listener, zmk_mouse_scroll_state_changed);
