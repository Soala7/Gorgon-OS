#include "serial.h"
#include "task.h"
#include "../x86_64/cpu/pic.h"
#include "../x86_64/cpu/timer.h"

void kernel_main()
{
    serial_init();
    task_init();
    task_t *test_task = task_create();
    task_t *test_task_2 = task_create();

    if (test_task != 0)
    {
        serial_write_str("Test task created: ");
        serial_write_hex(test_task->id);
        serial_write_str("\n");
    }
    if (test_task_2 != 0)
    {
        serial_write_str("Test task created 2: ");
        serial_write_hex(test_task_2->id);
        serial_write_str("\n");
    }
    serial_write_str("Kernel started.\n");
    pic_remap();
    timer_init();
    serial_write_str("PIC and PIT initialized.\n");
    serial_write_str("Interrupts enabled.\n");
    __asm__ volatile("sti");
    unsigned long last_tick = 0;

    for (;;)
    {
        __asm__ volatile("hlt");
        if (timer_get_ticks() - last_tick >= 100)
        {
            last_tick = timer_get_ticks();

            serial_write_str("Timer ticks: ");
            serial_write_hex(timer_get_ticks());
            serial_write_str("\n");
        }
    }
}