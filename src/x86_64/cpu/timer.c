#include "timer.h"
#include "pit.h"
#include "../../kernel/task.h"

volatile uint64_t timer_ticks = 0;

void timer_init(void){
    pit_init();
}

task_t* timer_tick(interrupt_context_t *context){
    timer_ticks++;
    return task_schedule(context);
}

uint64_t timer_get_ticks(void){
    return timer_ticks;
}