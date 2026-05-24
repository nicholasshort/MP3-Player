#include "buttons.h"
#include "main.h"
#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <stddef.h>

#define BUTTON_EVENTS_BUFFER_LEN 5

#define BUTTON_DEBOUNCE_EDGE_THRESHOLD_MS   50
#define BUTTON_LONG_PRESS_THRESHOLD_MS      500
#define BUTTON_SHORT_PRESS_THRESHOLD_MS     50

typedef struct {
    button_event_t button_events[BUTTON_EVENTS_BUFFER_LEN];
    uint8_t head;
    uint8_t len;
} buttons_event_list_t;

static buttons_event_list_t buttons_event_list = {0};

typedef struct {
    bool     stable_pressed;
    bool     edge_debounce_active;
    uint32_t candidate_edge_tick;
    uint32_t last_confirmed_candidate_edge_tick;
} button_state_t;

static button_state_t buttons_state[BUTTON_ID_COUNT] = {0};

static GPIO_TypeDef* const button_id_to_gpio_port_map[BUTTON_ID_COUNT] = {
    [BUTTON_ID_PLAY]        = BUTTON_PLAY_GPIO_Port,
    [BUTTON_ID_NEXT]        = BUTTON_NEXT_GPIO_Port,
    [BUTTON_ID_MENU]        = BUTTON_MENU_GPIO_Port,
    [BUTTON_ID_PREV_PWR]    = BUTTON_PREV_PWR_GPIO_Port
};

static const uint16_t button_id_to_gpio_pin_map[BUTTON_ID_COUNT] = {
    [BUTTON_ID_PLAY]        = BUTTON_PLAY_Pin,
    [BUTTON_ID_NEXT]        = BUTTON_NEXT_Pin,
    [BUTTON_ID_MENU]        = BUTTON_MENU_Pin,
    [BUTTON_ID_PREV_PWR]    = BUTTON_PREV_PWR_Pin
};

static void buttons_add_event_to_list(button_event_t event) {
    if (buttons_event_list.len >= BUTTON_EVENTS_BUFFER_LEN) 
        return;
    uint8_t add_event_index = (buttons_event_list.head + buttons_event_list.len) % BUTTON_EVENTS_BUFFER_LEN;
    buttons_event_list.button_events[add_event_index] = event;
    buttons_event_list.len++;
}

static bool buttons_read_raw(button_id_e button) {

    return (HAL_GPIO_ReadPin(button_id_to_gpio_port_map[button], button_id_to_gpio_pin_map[button]) == GPIO_PIN_RESET); // Active low

}

void buttons_init(void) {

    buttons_event_list.head = 0;
    buttons_event_list.len  = 0;

    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < BUTTON_ID_COUNT; i++) {
        buttons_state[i].stable_pressed = buttons_read_raw((button_id_e)i);
        buttons_state[i].edge_debounce_active = false;
        buttons_state[i].candidate_edge_tick = 0;
        buttons_state[i].last_confirmed_candidate_edge_tick = now;
    }

}

void buttons_update_state_poll_1ms(void) {

    uint32_t now = HAL_GetTick();

    for (uint8_t i = 0; i < BUTTON_ID_COUNT; i++) {
        bool curr_pressed = buttons_read_raw((button_id_e)i);

        // If change in button press
        if (curr_pressed != buttons_state[i].stable_pressed) {

            if (!buttons_state[i].edge_debounce_active) {
                buttons_state[i].edge_debounce_active = true;
                buttons_state[i].candidate_edge_tick = now;
            }
            
            uint32_t edge_stable_time = (now - buttons_state[i].candidate_edge_tick);
            if (edge_stable_time >= BUTTON_DEBOUNCE_EDGE_THRESHOLD_MS) {

                buttons_state[i].stable_pressed = curr_pressed;

                // Rising Edge
                if (curr_pressed) {
                    
                }
                    

                // Falling Edge
                else {
                    uint32_t press_time = (buttons_state[i].candidate_edge_tick - buttons_state[i].last_confirmed_candidate_edge_tick);
                    if (press_time >= BUTTON_LONG_PRESS_THRESHOLD_MS) {
                        button_event_t event = {
                            .button_id = (button_id_e)i,
                            .type      = BUTTON_EVENT_LONG_PRESS
                        };
                        buttons_add_event_to_list(event);
                    }
                    else if (press_time >= BUTTON_SHORT_PRESS_THRESHOLD_MS) {
                        button_event_t event = {
                            .button_id = (button_id_e)i,
                            .type      = BUTTON_EVENT_SHORT_PRESS
                        };
                        buttons_add_event_to_list(event);
                    }
                }
                buttons_state[i].last_confirmed_candidate_edge_tick = buttons_state[i].candidate_edge_tick;
                buttons_state[i].edge_debounce_active = false;
                buttons_state[i].candidate_edge_tick = 0;
            }
        }
        else {
            buttons_state[i].edge_debounce_active = false;
            buttons_state[i].candidate_edge_tick = 0;
        }
    }

}

bool buttons_get_event(button_event_t* event) {

    if (event == NULL)
        return false;
    
    if (buttons_event_list.len <= 0)
        return false;

    *event = buttons_event_list.button_events[buttons_event_list.head];
    buttons_event_list.head = (buttons_event_list.head + 1) % BUTTON_EVENTS_BUFFER_LEN;
    buttons_event_list.len--;
    
    return true;

}

bool buttons_is_pressed(button_id_e button) {

    if (button >= BUTTON_ID_COUNT)
        return false;

    return buttons_state[button].stable_pressed;

}