global long_mode_start

extern kernel_main
extern idt_load
extern multiboot_info
extern tss_init
extern gdt_tss_descriptor

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

    ; Patch the TSS descriptor base and load TR, before entering
    ; the C kernel.
    ; gdt_tss_descriptor's address is loaded here as a plain
    ; 64-bit absolute immediate (no [rel ...], no RIP-relative
    ; displacement) - this code is still executing in the low
    ; bootstrap region, so the load itself is trivial, and an
    ; absolute mov-immediate has no PC32-style distance limit
    ; regardless of where the target symbol lives. tss_init()
    ; receives the real physical address as a parameter instead
    ; of trying to derive it itself from high-half C.
    mov rdi, gdt_tss_descriptor
    mov rax, tss_init
    call rax

    ; Enter the C kernel
    mov edi, [rel multiboot_info]
    mov rax, kernel_main
    call rax

.hang:
    cli
    hlt
    jmp .hang