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

#define RTC_INPUT_FREQ 32768
#define RTC_FREQ_TO_PRESCALER(FREQ) (uint16_t)(((RTC_INPUT_FREQ) / (FREQ)) - 1)

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#define ERR_CHECK(err, msg)                                                                        \
    if (err != NRFX_SUCCESS)                                                                       \
    LOG_ERR(msg " - %d\n", err)

const nrfx_rtc_t rtc = NRFX_RTC_INSTANCE(2);
nrf_ppi_channel_t m_timer_saadc_ppi_channel;

static void rtc_handler(nrfx_rtc_int_type_t int_type) {
    nrfx_err_t err_code;

    switch (int_type) {
    case NRFX_RTC_INT_TICK:
        /* LOG_DBG("tick interrupt received\n"); */
        /* LOG_DBG("counter %u\n", nrfx_rtc_counter_get(&rtc)); */
        break;
    default:
        LOG_DBG("rtc interrupt %d\n", int_type);
        break;
    }
}
void rtc_init() {
    uint32_t err_code;

    // Initialize RTC instance
    nrfx_rtc_config_t config = NRFX_RTC_DEFAULT_CONFIG;
    /* config.prescaler = 65535; */
    config.prescaler = RTC_FREQ_TO_PRESCALER(20);
    LOG_DBG("Prescaler = %d\n", config.prescaler);

    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(rtc2)), DT_IRQ(DT_NODELABEL(rtc2), priority),
                nrfx_rtc_2_irq_handler, NULL, 0);

    err_code = nrfx_rtc_init(&rtc, &config, rtc_handler);
    ERR_CHECK(err_code, "rtc initialization error");

    // Enable tick event & interrupt
    nrfx_rtc_tick_enable(&rtc, true);

    nrfx_rtc_enable(&rtc);
}

void ppi_init(void) {
    /**** RTC -> ADC *****/
    // Trigger task sample from timer
    nrfx_err_t err_code = nrfx_ppi_channel_alloc(&m_timer_saadc_ppi_channel);
    ERR_CHECK(err_code, "PPI channel allocation error");

    err_code = nrfx_ppi_channel_assign(
        m_timer_saadc_ppi_channel, nrfx_rtc_event_address_get(&rtc, NRF_RTC_EVENT_TICK),
        nrf_saadc_task_address_get(DT_REG_ADDR(DT_NODELABEL(adc)), NRF_SAADC_TASK_SAMPLE));
    ERR_CHECK(err_code, " PPI channel assignment error");

    err_code = nrfx_ppi_channel_enable(m_timer_saadc_ppi_channel);
    ERR_CHECK(err_code, " PPI channel enable error");
}

#define NUM_ADC_CHANS 2

nrfx_saadc_channel_t adc_channels[NUM_ADC_CHANS] = {
    NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN4, 4),
    NRFX_SAADC_DEFAULT_CHANNEL_SE(NRF_SAADC_INPUT_AIN5, 5),
};

struct adc_sample {
    nrf_saadc_value_t samples[NUM_ADC_CHANS];
    int32_t timestamp;
};

int current_adc_buffer = 0;
struct adc_sample adc_samples[2];
#define CURRENT_BUFFER current_adc_buffer
#define DOUBLE_BUFFER ((current_adc_buffer + 1) % 2)

void adc_handler(const nrfx_saadc_evt_t *p_event) {
    switch (p_event->type) {
    case NRFX_SAADC_EVT_DONE: ///< Event generated when the buffer is filled with samples.
        /* LOG_DBG("NRFX_SAADC_EVT_DONE\n"); */
        // LOG_DBG("counter %u, value %u\n", nrfx_rtc_counter_get(&rtc),
        // adc_samples[CURRENT_BUFFER].samples);
        /* LOG_DBG("event data %u %u\n", p_event->data.done.p_buffer[0], */
        /* p_event->data.done.p_buffer[1]); */
        nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START);
        handle_joystick_report(p_event->data.done.p_buffer[0], p_event->data.done.p_buffer[1]);
        /* printk("start"); */
        break;
    case NRFX_SAADC_EVT_BUF_REQ: ///< Event generated when the next buffer for continuous conversion
                                 ///< is requested.
        /* LOG_DBG("NRFX_SAADC_EVT_BUF_REQ\n"); */
        nrfx_saadc_buffer_set(adc_samples[DOUBLE_BUFFER].samples, NUM_ADC_CHANS);
        current_adc_buffer = DOUBLE_BUFFER;
        /* nrf_saadc_task_trigger(NRF_SAADC, NRF_SAADC_TASK_START); */
        break;
    case NRFX_SAADC_EVT_READY: ///< Event generated when the first buffer is acquired by the
                               ///< peripheral and sampling can be started.
        /* LOG_DBG("NRFX_SAADC_EVT_READY\n"); */
        break;
    case NRFX_SAADC_EVT_FINISHED:
        /* LOG_DBG("NRFX_SAADC_EVT_FINISHED\n"); */
        break;
    default:
        LOG_DBG("GOT UNKOWN SAAdC EVENT %d", p_event->type);
        break;
    }
}

void adc_init(void) {
    nrfx_err_t err;
    nrfx_saadc_adv_config_t adc_adv_config = NRFX_SAADC_DEFAULT_ADV_CONFIG;
    adc_adv_config.start_on_end = true;

    IRQ_CONNECT(DT_IRQN(DT_NODELABEL(adc)), DT_IRQ(DT_NODELABEL(adc), priority),
                nrfx_saadc_irq_handler, NULL, 0);

    err = nrfx_saadc_init(DT_IRQ(DT_NODELABEL(adc), priority));
    ERR_CHECK(err, "nrfx_saadc init error");

    adc_channels[0].channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
    adc_channels[1].channel_config.reference = NRF_SAADC_REFERENCE_VDD4;
    adc_channels[0].channel_config.gain = NRF_SAADC_GAIN1_4;
    adc_channels[1].channel_config.gain = NRF_SAADC_GAIN1_4;
    err = nrfx_saadc_channels_config(adc_channels, NUM_ADC_CHANS);
    ERR_CHECK(err, "nrfx_saadc channel config error");

    err = nrfx_saadc_advanced_mode_set(0b00110000, SAADC_RESOLUTION_VAL_14bit, &adc_adv_config,
                                       adc_handler);
    ERR_CHECK(err, "saadc setting advanced mode");

    err = nrfx_saadc_buffer_set(&(adc_samples[CURRENT_BUFFER].samples[0]), NUM_ADC_CHANS);
    ERR_CHECK(err, "saadc buffer set");

    err = nrfx_saadc_mode_trigger();
    ERR_CHECK(err, "saadc trigger");
}

int joy_init(const struct device *dev) {
    LOG_DBG("joystick init");
    /* struct joy_data *drv_data = dev->data; */
    /* const struct joy_config *drv_cfg = dev->config; */

    rtc_init();
    ppi_init();
    adc_init();

    return 0;
}

#define JOY_INST(n)                                                                                \
    struct joy_data joy_data_##n;                                                                  \
    const struct joy_config joy_cfg_##n = {                                                        \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, min_on), (1), (DT_INST_PROP(n, min_on))),             \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, frequency), (1), (DT_INST_PROP(n, frequency))),       \
        COND_CODE_0(DT_INST_NODE_HAS_PROP(n, reverse), (1), (DT_INST_PROP(n, reverse))),           \
    };                                                                                             \
    DEVICE_DT_INST_DEFINE(n, joy_init, NULL, &joy_data_##n, &joy_cfg_##n, POST_KERNEL,             \
                          CONFIG_SENSOR_INIT_PRIORITY, NULL);

DT_INST_FOREACH_STATUS_OKAY(JOY_INST)
