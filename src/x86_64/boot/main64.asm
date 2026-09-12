global long_mode_start
extern kernel_main
extern idt_load

section .text
bits 64

long_mode_start:
    call idt_load
    ; Clear the segment registers
    mov ax, 0x10
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax

    call kernel_main

    ; Stop the CPU
    hlt