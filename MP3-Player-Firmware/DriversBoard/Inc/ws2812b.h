#ifndef WS2812B_H
#define WS2812B_H

#include <stdint.h>
#include <stdbool.h>

typedef enum {
    WS2812B_LED_LEFT = 0,
    WS2812B_LED_RIGHT,
    WS2812B_LED_COUNT,
} ws2812b_led_id_e;

typedef enum {
    WS2812B_OK = 0,
    WS2812B_ERR_INVALID_LED,
    WS2812B_ERR_BUSY,
    WS2812B_ERR_NOT_INITIALIZED,
    WS2812B_ERR_HAL,
} ws2812b_status_e;

ws2812b_status_e ws2812b_init(void);
ws2812b_status_e ws2812b_set_colour(ws2812b_led_id_e led, uint8_t r, uint8_t g, uint8_t b);
ws2812b_status_e ws2812b_set_brightness(ws2812b_led_id_e led, uint8_t brightness);
ws2812b_status_e ws2812b_clear(void);
ws2812b_status_e ws2812b_update(void);

#endif // WS2812B_H