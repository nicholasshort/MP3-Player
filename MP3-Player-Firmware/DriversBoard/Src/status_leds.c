#include "status_leds.h"
#include "gpio.h"
#include <stdint.h>
#include <stdbool.h>

#define FAST_BLINK_TOGGLE_TIME_MS 250
#define SLOW_BLINK_TOGGLE_TIME_MS 1000

typedef struct {
    bool led_on;
    status_led_mode_e mode;
    uint32_t last_toggle_time;
} led_state_t;

static GPIO_TypeDef* const led_id_to_gpio_port_map[STATUS_LED_COUNT] = {
    [STATUS_LED_GREEN] = LED_GREEN_GPIO_Port,
    [STATUS_LED_BLUE]  = LED_BLUE_GPIO_Port,
};

static const uint16_t led_id_to_gpio_pin_map[STATUS_LED_COUNT] = {
    [STATUS_LED_GREEN]  = LED_GREEN_Pin,
    [STATUS_LED_BLUE]   = LED_BLUE_Pin,
};

static led_state_t led_states[STATUS_LED_COUNT];

static inline void turn_led_on(status_led_id_e led) {

    if (led_states[led].led_on)
        return;
    
    HAL_GPIO_WritePin(led_id_to_gpio_port_map[led], led_id_to_gpio_pin_map[led], GPIO_PIN_SET);
    led_states[led].led_on = true;
    led_states[led].last_toggle_time = HAL_GetTick();

}

static inline void turn_led_off(status_led_id_e led) {

    if (!led_states[led].led_on)
        return;

    HAL_GPIO_WritePin(led_id_to_gpio_port_map[led], led_id_to_gpio_pin_map[led], GPIO_PIN_RESET);
    led_states[led].led_on = false;
    led_states[led].last_toggle_time = HAL_GetTick();

}

static inline void toggle_led(status_led_id_e led) {

    if (led_states[led].led_on)
        turn_led_off(led);
    else
        turn_led_on(led);

}

void status_leds_init(void) {

    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < STATUS_LED_COUNT; i++) {
        HAL_GPIO_WritePin(led_id_to_gpio_port_map[i], led_id_to_gpio_pin_map[i], GPIO_PIN_RESET);
        led_states[i].led_on = false;
        led_states[i].mode = STATUS_LED_MODE_OFF;
        led_states[i].last_toggle_time = now;
    }

}

void status_leds_set_mode(status_led_id_e led, status_led_mode_e mode) {

    if (led >= STATUS_LED_COUNT)
        return;
    
    led_states[led].mode = mode;

}

void status_leds_update_1ms(void) {

    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < STATUS_LED_COUNT; i++) {

        switch (led_states[i].mode) {
            case STATUS_LED_MODE_OFF:
                turn_led_off((status_led_id_e)i);
                break;
            case STATUS_LED_MODE_ON:
                turn_led_on((status_led_id_e)i);
                break;
            case STATUS_LED_MODE_BLINK_SLOW:
                if (now - led_states[i].last_toggle_time >= SLOW_BLINK_TOGGLE_TIME_MS)
                    toggle_led((status_led_id_e)i);
                break;
            case STATUS_LED_MODE_BLINK_FAST:
                if (now - led_states[i].last_toggle_time >= FAST_BLINK_TOGGLE_TIME_MS)
                    toggle_led((status_led_id_e)i);
                break;
            default:
                turn_led_off((status_led_id_e)i);
                led_states[i].mode = STATUS_LED_MODE_OFF;
                break; 
        }

    }

}