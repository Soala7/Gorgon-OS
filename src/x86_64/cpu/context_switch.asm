global context_switch

section .text
bits 64

context_switch:
    ; rdi = old cpu_context_t
    ; rsi = new cpu_context_t

    ; Save current registers
    mov [rdi + 0],   r15
    mov [rdi + 8],   r14
    mov [rdi + 16],  r13
    mov [rdi + 24],  r12
    mov [rdi + 32],  r11
    mov [rdi + 40],  r10
    mov [rdi + 48],  r9
    mov [rdi + 56],  r8
    mov [rdi + 64],  rbp
    mov [rdi + 72],  rdi
    mov [rdi + 80],  rsi
    mov [rdi + 88],  rdx
    mov [rdi + 96],  rcx
    mov [rdi + 104], rbx
    mov [rdi + 112], rax

    ; Save current instruction address
    mov rax, [rsp]
    mov [rdi + 120], rax

    ; Save current stack pointer
    mov [rdi + 144], rsp

    ; Load new registers
    mov r15, [rsi + 0]
    mov r14, [rsi + 8]
    mov r13, [rsi + 16]
    mov r12, [rsi + 24]
    mov r11, [rsi + 32]
    mov r10, [rsi + 40]
    mov r9,  [rsi + 48]
    mov r8,  [rsi + 56]
    mov rbp, [rsi + 64]
    mov rdx, [rsi + 88]
    mov rcx, [rsi + 96]
    mov rbx, [rsi + 104]

    ; Load new stack
    mov rsp, [rsi + 144]

    ; Load new instruction address
    mov rax, [rsi + 120]

    ; Jump into the new task
    jmp rax