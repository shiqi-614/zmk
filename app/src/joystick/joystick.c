#include <zephyr/logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/endpoints.h>
#include <zmk/mouse.h>
#include <zmk/hid.h>
#include <math.h>
#include "joystick.h"


#define M_PI 3.14159265358979323846
#define min(a, b) (((a) < (b)) ? (a) : (b))
#define max(a, b) (((a) > (b)) ? (a) : (b))
#define constrain(value, low, high) max(low, min(high, value))

// Ramp function that results in "deadzone" in the lower part of the range.
// (Not suited for negative K values).
// Input X: X axis as unit value.
// Input K: Factor as unit value.
// Original: (0,0)->(1,1)
// Result:   (0,0)->(k,0)->(1,1)
#define ramp_low(x, k) (x < k ? 0 : (x - k) * (1 / (1 - k)))
/* #define deadzone 0.12 */
/* #define step 40 */

#define radians(degrees) (degrees * M_PI / 180.0)

const int range = 1024 * 8 * 2;
const int center = range / 2;

float convert(int value) {
    if (value > range || value < 0) {
        return 0;
    }
    value = value - center;
    float res = (value * 1.0 / center);
    res = constrain(res, -1.0, 1.0);
    return res;
}

void send_joystick_report(struct joystick_data *data) {
    int x = data->x;
    int y = data->y;
    LOG_DBG("handle_joystick_report %d %d\n", (int)x, (int)y);
    zmk_hid_mouse_movement_set(x, y);
    zmk_endpoints_send_mouse_report();
}

void get_joystick_report(int adcX, int adcY, struct joystick_data *data) {
    struct joystick_config config = data->config;

    float x = convert(adcX);
    float y = convert(adcY);

    float angle = atan2(x, -y) * (180 / M_PI);
    float radius = sqrt(powf(x, 2) + powf(y, 2));
    radius = constrain(radius, 0, 1.0);
    radius = ramp_low(radius, config.deadzone);
    x = sin(radians(angle)) * radius;
    y = -cos(radians(angle)) * radius;

    data->x = (x * config.move_step);
    data->y = (y * config.move_step);

}
