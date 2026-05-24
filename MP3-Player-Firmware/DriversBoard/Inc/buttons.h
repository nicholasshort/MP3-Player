#ifndef BUTTONS_H
#define BUTTONS_H

#include <stdbool.h>

typedef enum {
    BUTTON_ID_PLAY = 0,
    BUTTON_ID_NEXT,
    BUTTON_ID_MENU,
    BUTTON_ID_PREV_PWR,
    BUTTON_ID_COUNT,
} button_id_e;

typedef enum {
    BUTTON_EVENT_SHORT_PRESS = 0,
    BUTTON_EVENT_LONG_PRESS,
    BUTTON_EVENT_DOUBLE_PRESS
} button_event_type_e;

typedef struct {
    button_id_e         button_id;
    button_event_type_e type;
} button_event_t;

void buttons_init(void);
void buttons_update_state_poll_1ms(void);
bool buttons_get_event(button_event_t* event);
bool buttons_is_pressed(button_id_e button);

#endif // BUTTONS_H