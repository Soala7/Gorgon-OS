global long_mode_start

extern kernel_main
extern idt_load
extern multiboot_info

section .boot.text
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
    mov rax, idt_load
    call rax

    ; Enter the C kernel
    mov edi, [rel multiboot_info]
    mov rax, kernel_main
    call rax

.hang:
    cli
    hlt
    jmp .hang