#include "task.h"
#include "serial.h"

static void test_task_1_entry(void)
{
    for (;;)
    {
        serial_write_str("Task 1 is running.\n");

        for (volatile uint64_t i = 0; i < 10000000; i++)
            ;
    }
}

static void test_task_2_entry(void)
{
    for (;;)
    {
        serial_write_str("Task 2 is running.\n");

        for (volatile uint64_t i = 0; i < 10000000; i++)
            ;
    }
}

static task_t tasks[MAX_TASKS];

static uint64_t task_count = 0;
static uint64_t current_task = 0;
static uint64_t next_task_id = 1;
static uint8_t scheduler_started = 0;

void task_init(void)
{
    task_count = 0;
    current_task = 0;
    next_task_id = 1;
    scheduler_started = 0;
}

task_t *task_create(void)
{
    if (task_count >= MAX_TASKS)
        return 0;

    task_t *task = &tasks[task_count];

    task->id = next_task_id++;
    task->state = TASK_READY;

    for (uint64_t i = 0; i < TASK_STACK_SIZE; i++)
        task->stack[i] = 0;

    task->context.r15 = 0;
    task->context.r14 = 0;
    task->context.r13 = 0;
    task->context.r12 = 0;
    task->context.r11 = 0;
    task->context.r10 = 0;
    task->context.r9  = 0;
    task->context.r8  = 0;
    task->context.rbp = 0;
    task->context.rdi = 0;
    task->context.rsi = 0;
    task->context.rdx = 0;
    task->context.rcx = 0;
    task->context.rbx = 0;
    task->context.rax = 0;

    if (task->id == 1)
        task->context.rip = (uint64_t)test_task_1_entry;
    else
        task->context.rip = (uint64_t)test_task_2_entry;

    task->context.cs = 0x08;
    task->context.rflags = 0x202;

    uint64_t stack_top =
        (uint64_t)&task->stack[TASK_STACK_SIZE];

    stack_top &= ~0xFULL;

    task->context.rsp = stack_top;
    task->context.ss = 0x10;

    task_count++;

    return task;
}

task_t *task_schedule(interrupt_context_t *context)
{
    if (context == 0 || task_count == 0)
        return 0;

    if (!scheduler_started)
    {
        scheduler_started = 1;
        current_task = 0;

        tasks[current_task].state = TASK_RUNNING;

        return &tasks[current_task];
    }

    task_t *current = &tasks[current_task];

    current->context.r15 = context->r15;
    current->context.r14 = context->r14;
    current->context.r13 = context->r13;
    current->context.r12 = context->r12;
    current->context.r11 = context->r11;
    current->context.r10 = context->r10;
    current->context.r9  = context->r9;
    current->context.r8  = context->r8;
    current->context.rbp = context->rbp;
    current->context.rdi = context->rdi;
    current->context.rsi = context->rsi;
    current->context.rdx = context->rdx;
    current->context.rcx = context->rcx;
    current->context.rbx = context->rbx;
    current->context.rax = context->rax;

    current->context.rip = context->rip;
    current->context.cs = context->cs;
    current->context.rflags = context->rflags;

    current->state = TASK_READY;

    current_task++;

    if (current_task >= task_count)
        current_task = 0;

    task_t *next = &tasks[current_task];

    next->state = TASK_RUNNING;

    return next;
}
