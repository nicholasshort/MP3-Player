#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum
{
    BUTTON_1 = 0,
    BUTTON_2,
    BUTTON_3,
    BUTTON_4,
    BUTTON_COUNT
} button_id_t;

typedef struct
{
    bool button_1;
    bool button_2;
    bool button_3;
    bool button_4;
} button_state_t;

void buttons_init(void);
void buttons_poll(void);

bool buttons_is_pressed(button_id_t button);
button_state_t buttons_get_state(void);