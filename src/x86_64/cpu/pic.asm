global pic_remap
global pic_send_eoi

section .text
bits 64

pic_remap:
    mov al, 0x11
    out 0x20, al
    out 0x80, al
    out 0xA0, al
    out 0x80, al

    mov al, 0x20
    out 0x21, al
    out 0x80, al

    mov al, 0x28
    out 0xA1, al
    out 0x80, al

    mov al, 0x04
    out 0x21, al
    out 0x80, al

    mov al, 0x02
    out 0xA1, al
    out 0x80, al

    mov al, 0x01
    out 0x21, al
    out 0x80, al
    out 0xA1, al
    out 0x80, al

    mov al, 0xFC
    out 0x21, al
    out 0x80, al

    mov al, 0xFF
    out 0xA1, al
    out 0x80, al

    ret

pic_send_eoi:
    cmp al, 8
    jb .master

    mov al, 0x20
    out 0xA0, al

.master:
    mov al, 0x20
    out 0x20, al
    ret