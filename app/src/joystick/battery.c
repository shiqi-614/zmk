#include <zephyr/logging/log.h>
#include <zephyr/bluetooth/services/bas.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event_manager.h>
#include <zmk/battery.h>
#include <zmk/events/battery_state_changed.h>

static uint8_t last_state_of_charge = 0;

#define ADC_RES_12BIT                   4096                                   
#define ADC_REF_VOLTAGE_IN_MILLIVOLTS   600                                     /**< Reference voltage (in milli volts) used by ADC while doing conversion. */
#define ADC_PRE_SCALING_COMPENSATION    2 * 5                                       /**< The ADC is configured to use VDD with 1/3 prescaling as input. And hence the result of conversion is to be multiplied by 3 to get the actual value of the battery voltage.*/

/**@brief Macro to convert the result of ADC conversion in millivolts.
 *
 * @param[in]  ADC_VALUE   ADC result.
 *
 * @retval     Result converted to millivolts.
 */
#define ADC_RESULT_IN_MILLI_VOLTS(ADC_VALUE)\
        ((((ADC_VALUE) * ADC_REF_VOLTAGE_IN_MILLIVOLTS) * ADC_PRE_SCALING_COMPENSATION) / ADC_RES_12BIT)



uint8_t lithium_ion_mv_to_pct(int16_t bat_mv) {
    // Simple linear approximation of a battery based off adafruit's discharge graph:
    // https://learn.adafruit.com/li-ion-and-lipoly-batteries/voltages
    if (bat_mv >= 4200) {
        return 100;
    } else if (bat_mv <= 3450) {
        return 0;
    }
    return bat_mv * 2 / 15 - 459;
}

void update_battery_level(int raw_battery) {
    int batt_lvl_in_milli_volts = ADC_RESULT_IN_MILLI_VOLTS(raw_battery);
    int percentage_batt_lvl = lithium_ion_mv_to_pct(batt_lvl_in_milli_volts);
    LOG_DBG("raw data %d, batt_lvl_in_milli_volts %d battery level %d\n", raw_battery, batt_lvl_in_milli_volts, percentage_batt_lvl);
    if (last_state_of_charge != percentage_batt_lvl) {
        LOG_DBG("battery level %d\n", percentage_batt_lvl);
        last_state_of_charge = percentage_batt_lvl;

        int rc = bt_bas_set_battery_level(last_state_of_charge);

        if (rc != 0) {
            LOG_WRN("Failed to set BAS GATT battery level (err %d)", rc);
        }
        rc = raise_zmk_battery_state_changed(
            (struct zmk_battery_state_changed){.state_of_charge = last_state_of_charge});
    }
}
