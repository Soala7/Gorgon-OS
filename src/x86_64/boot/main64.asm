global long_mode_start

extern kernel_main
extern idt_load

section .text
bits 64

long_mode_start:
    ; Load kernel data segments first
    mov ax, 0x10
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax

    ; Install the IDT
    call idt_load

    ; Enter the C kernel
    call kernel_main

.hang:
    cli
    hlt
    jmp .hang