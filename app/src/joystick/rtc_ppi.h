
#include <nrfx_ppi.h>
#include <nrfx_clock.h>
#include <nrfx_rtc.h>
#include <nrfx_saadc.h>

#define ERR_CHECK(err, msg)                                                                        \
    if (err != NRFX_SUCCESS)                                                                       \
    LOG_ERR(msg " - %d\n", err)

#define RTC_INPUT_FREQ 32768
#define RTC_FREQ_TO_PRESCALER(FREQ) (uint16_t)(((RTC_INPUT_FREQ) / (FREQ)) - 1)

nrfx_rtc_t rtc_init(int hertz);
void ppi_init(nrfx_rtc_t rtc);
