#include "ws2812b.h"
#include "tim.h"
#include "stm32f4xx_hal_tim.h"
#include <string.h>

#define TIMER_HANDLE  &htim2
#define TIMER_CHANNEL TIM_CHANNEL_1

// Timer couter counts from 0 to 119. Completing one cycle at every 1.25us (800kHz). LEDs expect data bits every 1.25us.
// RGB Bits sent by varying duty cycle for 1s and 0s. Duty cycle can be determine by N/120 where N is CCR register value.
#define ZERO_BIT_DUTY_CYCLE     38 // 32% duty cycle
#define ONE_BIT_DUTY_CYCLE      77 // 64% duty cycle

#define RESET_BITS_PER_RESET_CYCLE 50 // 40 x 1.25us = 50us. Use 50 reset cycles for extra margin at 62.5us

#define NUM_BITS_RGB 24

typedef struct {
    uint8_t r;
    uint8_t g;
    uint8_t b;
} colour_t;

typedef struct {
    colour_t colour;
    uint8_t brightness;
    uint32_t duty_cycle_bits[NUM_BITS_RGB];
} led_data_t;

static led_data_t left_led_data;
static led_data_t right_led_data;

#define DMA_BUFFER_LEN (NUM_BITS_RGB * 2 + RESET_BITS_PER_RESET_CYCLE)
static uint32_t dma_buffer[DMA_BUFFER_LEN]; 

static volatile bool dma_in_progress;

static void update_duty_cycle_bits(led_data_t* led) {

    for (uint8_t i = 0; i < 3; i++) {

        uint8_t colour_to_transmit = 0;
        if (i == 0)
            colour_to_transmit = led->colour.g;
        else if(i == 1)
            colour_to_transmit = led->colour.r;
        else
            colour_to_transmit = led->colour.b;

        colour_to_transmit = (uint8_t)(((uint16_t)colour_to_transmit * led->brightness) / 255u);
        
        for (uint8_t j = 0; j < 8; j++) {

            if (colour_to_transmit & (1u << (7-j)))
                led->duty_cycle_bits[i*8 + j] = ONE_BIT_DUTY_CYCLE;
            else
                led->duty_cycle_bits[i*8 + j] = ZERO_BIT_DUTY_CYCLE;

        }

    }

}

ws2812b_status_e ws2812b_init(void) {

    memset(&left_led_data, 0, sizeof(left_led_data));
    memset(&right_led_data, 0, sizeof(right_led_data));
    memset(dma_buffer, 0, sizeof(dma_buffer));

    left_led_data.brightness  = 255;
    right_led_data.brightness = 255;

    update_duty_cycle_bits(&left_led_data);
    update_duty_cycle_bits(&right_led_data);

    dma_in_progress = false;

    return WS2812B_OK;

}

ws2812b_status_e ws2812b_set_colour(ws2812b_led_id_e led, uint8_t r, uint8_t g, uint8_t b) {

    if (led >= WS2812B_LED_COUNT)
        return WS2812B_ERR_INVALID_LED;

    if (led == WS2812B_LED_LEFT) {
        left_led_data.colour.r = r;
        left_led_data.colour.g = g;
        left_led_data.colour.b = b;
        update_duty_cycle_bits(&left_led_data);
    } 
    else {
        right_led_data.colour.r = r;
        right_led_data.colour.g = g;
        right_led_data.colour.b = b;
        update_duty_cycle_bits(&right_led_data);
    }

    return WS2812B_OK;

}

ws2812b_status_e ws2812b_set_brightness(ws2812b_led_id_e led, uint8_t brightness) {

    if (led >= WS2812B_LED_COUNT)
        return WS2812B_ERR_INVALID_LED;

    if (led == WS2812B_LED_LEFT) {
        left_led_data.brightness = brightness;
        update_duty_cycle_bits(&left_led_data);
    }
        
    else {
        right_led_data.brightness = brightness;
        update_duty_cycle_bits(&right_led_data);
    }

    return WS2812B_OK;
    
}

ws2812b_status_e ws2812b_clear(void) {

    if (dma_in_progress)
        return WS2812B_ERR_BUSY;

    memset(&left_led_data, 0, sizeof(left_led_data));
    memset(&right_led_data, 0, sizeof(right_led_data));

    left_led_data.brightness  = 255;
    right_led_data.brightness = 255;

    update_duty_cycle_bits(&left_led_data);
    update_duty_cycle_bits(&right_led_data);

    return ws2812b_update();

}

ws2812b_status_e ws2812b_update(void) {

    if (dma_in_progress)
        return WS2812B_ERR_BUSY;
    
    memcpy(dma_buffer, left_led_data.duty_cycle_bits, NUM_BITS_RGB * sizeof(dma_buffer[0]));
    memcpy(&dma_buffer[NUM_BITS_RGB], right_led_data.duty_cycle_bits, NUM_BITS_RGB * sizeof(dma_buffer[0]));

    dma_in_progress = true;
    HAL_StatusTypeDef err = HAL_TIM_PWM_Start_DMA(TIMER_HANDLE, TIMER_CHANNEL, dma_buffer, DMA_BUFFER_LEN);
    if (err != HAL_OK) {
        dma_in_progress = false;
        return WS2812B_ERR_HAL;
    }

    return WS2812B_OK;

}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {

    if (htim->Instance != TIM2) {
        return;
    }

    HAL_TIM_PWM_Stop_DMA(TIMER_HANDLE, TIMER_CHANNEL);
    dma_in_progress = false;

}