#ifndef TASK_H
#define TASK_H

#include <stdint.h>

#define MAX_TASKS 16
#define TASK_STACK_SIZE 16384

typedef enum {
    TASK_READY,
    TASK_RUNNING,
    TASK_BLOCKED,
    TASK_TERMINATED
} task_state_t;

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
    uint64_t rsp;
    uint64_t ss;
} task_context_t;

typedef struct {
    uint64_t id;
    task_state_t state;
    uint8_t stack[TASK_STACK_SIZE];
    task_context_t context;
} task_t;

typedef struct {
    uint64_t r15;
    uint64_t r14;
    uint64_t r13;
    uint64_t r12;
    uint64_t r11;
    uint64_t r10;
    uint64_t r9;
    uint64_t r8;
    uint64_t rbp;
    uint64_t rdi;
    uint64_t rsi;
    uint64_t rdx;
    uint64_t rcx;
    uint64_t rbx;
    uint64_t rax;

    uint64_t vector;
    uint64_t rip;
    uint64_t cs;
    uint64_t rflags;
} interrupt_context_t;

void task_init(void);
task_t *task_create(void);
task_t *task_schedule(interrupt_context_t *context);

#endif
