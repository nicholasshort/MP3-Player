#ifndef TIMING_H
#define TIMING_H

#include <stdint.h>
#include <stdbool.h>

// Start TIM5
void timing_init(void); // TIM5 ticks at 1 MHz

// Poll-based timer
typedef uint32_t timer_t;

void timer_start(timer_t* timer, uint32_t timeout_ms);
bool timer_timeout(const timer_t* timer);
void timer_clear(timer_t* timer);

// Stopwatch
typedef uint32_t stopwatch_t;

void stopwatch_start(stopwatch_t* stopwatch);
uint32_t stopwatch_end(const stopwatch_t* stopwatch);
void stopwatch_clear(stopwatch_t* stopwatch);

#endif // TIMING_H