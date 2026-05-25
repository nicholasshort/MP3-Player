#ifndef STATUS_LEDS_H
#define STATUS_LEDS_H

typedef enum {
    STATUS_LED_MODE_OFF = 0,
    STATUS_LED_MODE_ON,
    STATUS_LED_MODE_BLINK_SLOW,
    STATUS_LED_MODE_BLINK_FAST,
} status_led_mode_e;

typedef enum {
    STATUS_LED_GREEN = 0,
    STATUS_LED_BLUE,
    STATUS_LED_COUNT,
} status_led_id_e;

void status_leds_init(void);
void status_leds_set_mode(status_led_id_e led, status_led_mode_e mode);
void status_leds_update_1ms(void);

#endif  // STATUS_LEDS_H