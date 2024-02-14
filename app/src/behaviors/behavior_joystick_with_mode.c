/*
 * Copyright (c) 2023 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT ergocai_behavior_joystick_with_mode

#include <zephyr/device.h>

#include <drivers/behavior.h>

#include "behavior_joystick_with_mode.h"

int behavior_joystick_with_mode_accept_data(struct zmk_behavior_binding *binding,
                                            struct zmk_behavior_binding_event event,
                                            const struct zmk_sensor_config *sensor_config,
                                            size_t channel_data_size,
                                            const struct zmk_sensor_channel_data *channel_data) {
    LOG_DBG("behavior_joystick_with_mode_accept_data");
}

int behavior_joystick_with_mode_process(struct zmk_behavior_binding *binding,
                                        struct zmk_behavior_binding_event event,
                                        enum behavior_sensor_binding_process_mode mode) {
    LOG_DBG("behavior_joystick_with_mode_process");
}

static const struct behavior_driver_api behavior_joystick_with_mode_driver_api = {
    .sensor_binding_accept_data = behavior_joystick_with_mode_accept_data,
    .sensor_binding_process = behavior_joystick_with_mode_process};

static int behavior_joystick_with_mode_init(const struct device *dev) { return 0; };

#define SENSOR_JOYSTICK_WITH_MODE_INST(n)                                                          \
    static struct behavior_sensor_rotate_config behavior_joystick_with_mode_config_##n = {         \
        .mode = DT_INST_PROP(n, mode),                                                             \
    };                                                                                             \
    static struct behavior_sensor_rotate_data behavior_joystick_with_mode_data_##n = {};           \
    DEVICE_DT_INST_DEFINE(                                                                         \
        n, behavior_joystick_with_mode_init, NULL, &behavior_joystick_with_mode_data_##n,          \
        &behavior_joystick_with_mode_config_##n, APPLICATION, CONFIG_KERNEL_INIT_PRIORITY_DEFAULT, \
        &behavior_joystick_with_mode_driver_api);

DT_INST_FOREACH_STATUS_OKAY(SENSOR_JOYSTICK_WITH_MODE_INST)
