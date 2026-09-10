global idt_load
global divide_by_zero_handler

section .text

bits 64

idt_load:
    lea rax, [rel divide_by_zero_handler]

    ; Handler address bits 0-15
    mov word [idt + 0], ax

    ; Code segment selector
    mov word [idt + 2], 0x08

    ; IST
    mov byte [idt + 4], 0

    ; Interrupt gate, present, DPL 0
    mov byte [idt + 5], 0x8E

    ; Reserved
    mov word [idt + 6], 0

    ; Handler address bits 16-31
    shr rax, 16
    mov word [idt + 6], ax

    ; Handler address bits 32-63
    shr rax, 16
    mov dword [idt + 8], eax

    ; Reserved
    mov dword [idt + 12], 0

    lidt [idt_pointer]
    ret

divide_by_zero_handler:
    cli
    hlt


section .rodata

align 16

idt:
    times 256 dq 0
    times 256 dq 0

idt_pointer:
    dw $ - idt - 1
    dq idt
;This creates space for 256 IDT entries and provides idt_load, which loads the table into the CPU with lidt