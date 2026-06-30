#include "timing.h"
#include "tim.h"

#define TIMER_HANDLE (&htim5)

#define TIMING_TIMER_CLEAR 0

void timing_init(void) {

    HAL_TIM_Base_Start(TIMER_HANDLE);

}

void timer_start(timer_t* timer, uint32_t timeout_ms) {

    *timer = __HAL_TIM_GET_COUNTER(TIMER_HANDLE) + timeout_ms * 1000u;

}

bool timer_timeout(const timer_t* timer) {

    if (*timer == TIMING_TIMER_CLEAR)
        return false;

    return ((int32_t)(__HAL_TIM_GET_COUNTER(TIMER_HANDLE) - *timer)) >= 0; // Wrap Safe (TIM5 wraps once every 71 min)

}

void timer_clear(timer_t* timer) {

    *timer = TIMING_TIMER_CLEAR;

}

#define TIMING_STOPWATCH_CLEAR 0

void stopwatch_start(stopwatch_t* stopwatch) {

    *stopwatch = __HAL_TIM_GET_COUNTER(TIMER_HANDLE);

}

uint32_t stopwatch_end(const stopwatch_t* stopwatch) {

    if (*stopwatch == TIMING_STOPWATCH_CLEAR)
        return 0;

    return (__HAL_TIM_GET_COUNTER(TIMER_HANDLE) - *stopwatch);

}

void stopwatch_clear(stopwatch_t* stopwatch) {

    *stopwatch = TIMING_STOPWATCH_CLEAR;

}