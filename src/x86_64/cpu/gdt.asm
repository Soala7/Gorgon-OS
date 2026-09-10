global gdt_load

section .text
bits 32

gdt_load:
    lgdt [gdt_pointer]
    ret


section .rodata

gdt:
    ; Null descriptor
    dq 0

    ; 64-bit code segment
    dq (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)

    ; Data segment
    dq (1 << 44) | (1 << 47)

gdt_pointer:
    dw $ - gdt - 1
    dd gdt