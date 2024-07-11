#include "rtc_ppi.h"
#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

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

void ppi_init(nrfx_rtc_t rtc) {
    /**** RTC -> ADC *****/
    // Trigger task sample from timer
    LOG_DBG("PPI init");
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

