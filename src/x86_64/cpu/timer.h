#pragma once

#include <stdint.h>
#include "../../kernel/task.h"

extern volatile uint64_t timer_ticks;

void timer_init(void);
task_t* timer_tick(interrupt_context_t *context);
uint64_t timer_get_ticks(void);