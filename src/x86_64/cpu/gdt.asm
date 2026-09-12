global gdt_load

section .text
bits 32

gdt_load:
    lgdt [gdt_pointer]
    ret

section .rodata

gdt:
    dq 0
    dq (1 << 41) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)
    dq (1 << 41) | (1 << 44) | (1 << 47)

gdt_pointer:
    dw $ - gdt - 1
    dd gdt