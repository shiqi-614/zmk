/*
 * Copyright (c) 2020 The ZMK Contributors
 *
 * SPDX-License-Identifier: MIT
 */

#define DT_DRV_COMPAT ergocai_joystick

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdlib.h>
#include <nrfx_ppi.h>
#include <nrfx_clock.h>
#include <nrfx_rtc.h>
#include <nrfx_saadc.h>

#include "joystick.h"

#define NUM_ADC_CHANS 2
#define RTC_INPUT_FREQ 32768
#define RTC_FREQ_TO_PRESCALER(FREQ) (uint16_t)(((RTC_INPUT_FREQ) / (FREQ)) - 1)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ERR_CHECK(err, msg)                                                                        \
    if (err != NRFX_SUCCESS)                                                                       \
    LOG_ERR(msg " - %d\n", err)

static const nrf_saadc_input_t ANALOG_INPUT_MAP[] = {
    NRF_SAADC_INPUT_AIN0, NRF_SAADC_INPUT_AIN1, NRF_SAADC_INPUT_AIN2, NRF_SAADC_INPUT_AIN3,
    NRF_SAADC_INPUT_AIN4, NRF_SAADC_INPUT_AIN5, NRF_SAADC_INPUT_AIN6, NRF_SAADC_INPUT_AIN7};

static struct joystick_data joy_data;


static void rtc_handler(nrfx_rtc_int_type_t int_type) {
    switch (int_type) {
    case NRFX_RTC_INT_TICK:
        break;
    default:
        LOG_DBG("rtc interrupt %d\n", int_type);
        break;
    }
}

nrfx_rtc_t rtc_init(int hertz) {
    uint32_t err_code;

    // Initialize RTC instance
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    /* config.prescaler = 65535; */
    config.prescaler = RTC_FREQ_TO_PRESCALER(hertz);
    LOG_DBG("Prescaler = %d\n", config.prescaler);

    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(rtc2)), DT_IRQ(DT_NODELABEL(rtc2), priority),
                nrfx_rtc_2_irq_handler, NULL, 0);

    const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);
    err_code = nrfx_rtc_init(&rtc, &config, rtc_handler);
    ERR_CHECK(err_code, "rtc initialization error");

    // Enable tick event & interrupt
    nrfx_rtc_tick_enable(&rtc, true);

    nrfx_rtc_enable(&rtc);
    return rtc;
}

// https://devzone.nordicsemi.com/f/nordic-q-a/72487/how-to-use-nrfx-saadc-driver-on-zephyr-rtos
void ppi_init(nrfx_rtc_t rtc) {
    /**** RTC -> ADC *****/
    // Trigger task sample from timer
    nrf_ppi_channel_t m_timer_saadc_ppi_channel;
    nrfx_err_t err_code = nrfx_ppi_channel_alloc(&m_timer_saadc_ppi_channel);
    ERR_CHECK(err_code, "PPI channel allocation error");

    err_code = nrfx_ppi_channel_assign(
        m_timer_saadc_ppi_channel, nrfx_rtc_event_address_get(&rtc, NRF_RTC_EVENT_TICK),
        nrf_saadc_task_address_get(DT_REG_ADDR(DT_NODELABEL(adc)), NRF_SAADC_TASK_SAMPLE));
    ERR_CHECK(err_code, " PPI channel assignment error");

    err_code = nrfx_ppi_channel_enable(m_timer_saadc_ppi_channel);
    ERR_CHECK(err_code, " PPI channel enable error");
}



struct adc_sample {
    nrf_saadc_value_t samples[NUM_ADC_CHANS];
    int32_t timestamp;
};

int current_adc_buffer = 0;
struct adc_sample adc_samples[2];
#define NEXT_BUFFER ((current_adc_buffer + 1) % 2)

void joystick_work(struct k_work *item) {
    struct joystick_data *data=
        CONTAINER_OF(item, struct joystick_data, work);
    send_joystick_report(joy_data);

}
void adc_handler(const nrfx_saadc_evt_t *p_event) {
    switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE: ///< Event generated when the buffer is filled with samples.
        nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
        int raw_x = p_event->data.done.p_buffer[0];
        int raw_y = p_event->data.done.p_buffer[1];
        get_joystick_report(raw_x, raw_y, &joy_data);
        if (joy_data.x != 0 || joy_data.y != 0) {
            k_work_submit(&joy_data.work);
        }
        break;
    case NRFX_SAADC_EVT_BUF_REQ: ///< Event generated when the next buffer for continuous conversion
                                 ///< is requested.
        nrfx_saadc_buffer_set(adc_samples[NEXT_BUFFER].samples, NUM_ADC_CHANS);
        current_adc_buffer = NEXT_BUFFER;
        break;
    case NRFX_SAADC_EVT_READY: ///< Event generated when the first buffer is acquired by the
                               ///< peripheral and sampling can be started.
        break;
    case NRFX_SAADC_EVT_FINISHED:
        break;
    default:
        LOG_DBG("GOT UNKOWN SAAdC EVENT %d", p_event->type);
        break;
    }
}

void adc_init(struct joystick_config config) {
    nrfx_err_t err;
    nrfx_saadc_adv_config_t adc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    adc_adv_config.start_on_end = true;

    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)), DT_IRQ(DT_NODELABEL(adc), priority),
                nrfx_saadc_irq_handler, NULL, 0);

    err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
    ERR_CHECK(err, "nrfx_saadc init error");


    nrfx_saadc_channel_t adc_channels[NUM_ADC_CHANS] = {
        NRFX_SAADC_DEFAULT_CHANNEL_SE(ANALOG_INPUT_MAP[config.x_channel_idx], config.x_channel_idx),
        NRFX_SAADC_DEFAULT_CHANNEL_SE(ANALOG_INPUT_MAP[config.y_channel_idx], config.y_channel_idx),
    };

    uint8_t channel_mask = 0;
    for (int i = 0; i < NUM_ADC_CHANS; i++) {
        adc_channels[i].channel_config.reference = config.adc_reference;
        adc_channels[i].channel_config.gain = config.adc_gain;

        channel_mask |= 1 << (adc_channels[i].channel_index);
    }

    err = nrfx_saadc_channels_config(adc_channels, NUM_ADC_CHANS);
    ERR_CHECK(err, "nrfx_saadc channel config error");

    err = nrfx_saadc_advanced_mode_set(channel_mask, SAADC_RESOLUTION_VAL_14bit, &adc_adv_config,
                                       adc_handler);
    ERR_CHECK(err, "saadc setting advanced mode");

    err = nrfx_saadc_buffer_set(&(adc_samples[current_adc_buffer].samples[0]), NUM_ADC_CHANS);
    ERR_CHECK(err, "saadc buffer set");

    err = nrfx_saadc_mode_trigger();
    ERR_CHECK(err, "saadc trigger");
}

int joy_init(const struct device *dev) {
    LOG_DBG("joystick init");
    const struct joystick_config *drv_cfg = dev->config;

    joy_data.config = *drv_cfg;
    k_work_init(&joy_data.work, joystick_work);

    nrfx_rtc_t rtc = rtc_init(drv_cfg->polling_rate_hertz);
    ppi_init(rtc);
    adc_init(joy_data.config);

    return 0;
}

#define JOY_INST(n)                                                                                \
    const struct joystick_config joy_cfg_##n = {                                                        \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, adc_x_channel), (4), (DT_INST_PROP(n, adc_x_channel))),           \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, adc_y_channel), (5), (DT_INST_PROP(n, adc_y_channel))),           \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, adc_channel_reference), (NRF_SAADC_REFERENCE_VDD4), (DT_INST_STRING_UNQUOTED(n, adc_channel_reference))),           \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, adc_channel_gain), (NRF_SAADC_GAIN1_4), (DT_INST_STRING_UNQUOTED(n, adc_channel_gain))),           \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, polling_rate_hertz), (20), (DT_INST_PROP(n, polling_rate_hertz))),       \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, move_step), (40), (DT_INST_PROP(n, move_step))),           \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, deadzone), (0.12), (DT_INST_STRING_UNQUOTED(n, deadzone))),             \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, joy_init, NULL, NULL, &joy_cfg_##n, POST_KERNEL,             \
                          CONFIG_SENSOR_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(JOY_INST)
