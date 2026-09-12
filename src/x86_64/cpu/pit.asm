global pit_init

section .text
bits 64

pit_init:
    mov al, 0x36
    out 0x43, al

    mov ax, 11931
    out 0x40, al
    mov al, ah
    out 0x40, al

    ret