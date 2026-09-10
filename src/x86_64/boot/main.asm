global start
extern long_mode_start
extern gdt_load
extern idt_load
extern divide_by_zero_handler

section .text
bits 32

start:
    ; Start the stack at its top so it grows downward
    mov esp, start_stack

    ; Check that the bootloader used Multiboot2
    call check_multiboot

    ; Check whether the CPU supports CPUID
    call check_cpuid

    ; Check whether the CPU supports 64-bit long mode
    call check_cpu_long

    ; Create the initial page tables
    call setup_page_table

    ; Enable paging and long mode
    call enable_page_table

    ; Load Gorgon's Global Descriptor Table
    call gdt_load

    ; Load Gorgon's Interrupt Descriptor Table
    call idt_load

    ; Jump into 64-bit code
    jmp 0x08:long_mode_start

    ; Should never be reached
    hlt


; ------------------------------------------------------------
; Check Multiboot2
; ------------------------------------------------------------

check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret

.no_multiboot:
    mov al, "M"
    jmp error


; ------------------------------------------------------------
; Check CPUID support
; ------------------------------------------------------------

check_cpuid:
    ; Save current EFLAGS
    pushfd
    pop eax

    mov ecx, eax

    ; Toggle the CPUID flag
    xor eax, 1 << 21

    push eax
    popfd

    ; Read EFLAGS again
    pushfd
    pop eax

    ; Restore original EFLAGS
    push ecx
    popfd

    ; If the bit could not be changed, CPUID is unavailable
    cmp eax, ecx
    je .no_cpuid

    ret

.no_cpuid:
    mov al, "C"
    jmp error


; ------------------------------------------------------------
; Check 64-bit long mode support
; ------------------------------------------------------------

check_cpu_long:
    ; Ask CPUID for the highest extended function supported
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    ; Get extended CPU feature information
    mov eax, 0x80000001
    cpuid

    ; Bit 29 of EDX = long mode support
    test edx, 1 << 29
    jz .no_long_mode

    ret

.no_long_mode:
    mov al, "L"
    jmp error


; ------------------------------------------------------------
; Create initial page tables
; ------------------------------------------------------------

setup_page_table:

    ; Page-table entries are physical addresses + flags
    mov eax, page_table_l3
    or eax, 0b11
    mov [page_table_l4], eax

    mov eax, page_table_l2
    or eax, 0b11
    mov [page_table_l3], eax

    ; Create 512 × 2 MiB pages = 1 GiB
    xor ecx, ecx

.loop:
    mov eax, 0x200000
    mul ecx

    ; Present + writable + huge-page flags
    or eax, 0b10000011

    mov [page_table_l2 + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .loop

    ret


; ------------------------------------------------------------
; Enable paging and long mode
; ------------------------------------------------------------

enable_page_table:

    ; Tell the CPU where the level-4 page table is
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable Physical Address Extension (PAE)
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable long mode through the EFER MSR
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret


; ------------------------------------------------------------
; Display an error code on the VGA text buffer
; ------------------------------------------------------------

error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f324f52
    mov dword [0xb8008], 0x4f204f20
    mov byte [0xb800a], al

    hlt


; ------------------------------------------------------------
; Uninitialized memory
; ------------------------------------------------------------

section .bss

align 4096

page_table_l4:
    resb 4096

page_table_l3:
    resb 4096

page_table_l2:
    resb 4096

; 16 KiB stack
start_stack:
    resb 4096 * 4

end_stack:

