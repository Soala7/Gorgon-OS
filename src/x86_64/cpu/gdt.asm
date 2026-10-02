global gdt_load
global gdt_tss_descriptor

section .boot.text
bits 32

gdt_load:
    lgdt [gdt_pointer]
    ret


section .boot.data

gdt:
    dq 0

    dq (1 << 41) | (1 << 43) | (1 << 44) | (1 << 47) | (1 << 53)

    dq (1 << 41) | (1 << 44) | (1 << 47)


gdt_tss_descriptor:
    dw 0x0067
    dw 0x0000
    db 0x00
    db 0x89
    db 0x00
    db 0x00
    dd 0x00000000
    dd 0x00000000


gdt_pointer:
    dw $ - gdt - 1
    dd gdt