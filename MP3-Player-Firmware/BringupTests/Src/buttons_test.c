#include "buttons_test.h"

#include "main.h"
#include "stm32f4xx_hal.h"

typedef struct
{
    GPIO_TypeDef *port;
    uint16_t pin;
    bool pressed;
} button_t;

static button_t buttons[BUTTON_COUNT] =
{
    [BUTTON_1] =
    {
        .port = BUTTON_1_GPIO_Port,
        .pin = BUTTON_1_Pin,
        .pressed = false,
    },
    [BUTTON_2] =
    {
        .port = BUTTON_2_GPIO_Port,
        .pin = BUTTON_2_Pin,
        .pressed = false,
    },
    [BUTTON_3] =
    {
        .port = BUTTON_3_GPIO_Port,
        .pin = BUTTON_3_Pin,
        .pressed = false,
    },
    [BUTTON_4] =
    {
        .port = BUTTON_4_GPIO_Port,
        .pin = BUTTON_4_Pin,
        .pressed = false,
    },
};

static bool button_read_pressed(GPIO_TypeDef *port, uint16_t pin)
{
    /*
     * Hardware has external pull-ups:
     * released = high
     * pressed  = low
     */
    return HAL_GPIO_ReadPin(port, pin) == GPIO_PIN_RESET;
}

void buttons_init(void)
{
    buttons_poll();
}

void buttons_poll(void)
{
    for (uint32_t i = 0; i < BUTTON_COUNT; i++)
    {
        buttons[i].pressed = button_read_pressed(buttons[i].port, buttons[i].pin);
    }
}

bool buttons_is_pressed(button_id_t button)
{
    if (button >= BUTTON_COUNT)
    {
        return false;
    }

    return buttons[button].pressed;
}

button_state_t buttons_get_state(void)
{
    button_state_t state =
    {
        .button_1 = buttons[BUTTON_1].pressed,
        .button_2 = buttons[BUTTON_2].pressed,
        .button_3 = buttons[BUTTON_3].pressed,
        .button_4 = buttons[BUTTON_4].pressed,
    };

    return state;
}