global start

extern long_mode_start
global multiboot_info
extern gdt_load

section .boot.text
bits 32

start:
    mov esp, end_stack
    mov ebp, esp

    mov [multiboot_info], ebx

    call check_multiboot
    call check_cpuid
    call check_cpu_long
    call setup_page_table
    call enable_page_table
    call enable_nxe
    call enable_write_protect
    call gdt_load

    ; long_mode_start is deliberately kept in the low bootstrap
    ; region so this 32-bit far jump remains valid.
    jmp 0x08:long_mode_start

    hlt


check_multiboot:
    cmp eax, 0x36d76289
    jne .no_multiboot
    ret

.no_multiboot:
    mov al, "M"
    jmp error


check_cpuid:
    pushfd
    pop eax
    mov ecx, eax

    xor eax, 1 << 21

    push eax
    popfd

    pushfd
    pop eax

    push ecx
    popfd

    cmp eax, ecx
    je .no_cpuid

    ret

.no_cpuid:
    mov al, "C"
    jmp error


check_cpu_long:
    mov eax, 0x80000000
    cpuid

    cmp eax, 0x80000001
    jb .no_long_mode

    mov eax, 0x80000001
    cpuid

    test edx, 1 << 29
    jz .no_long_mode

    ret

.no_long_mode:
    mov al, "L"
    jmp error


setup_page_table:
    ; --------------------------------------------------------
    ; Identity mapping
    ;
    ; PML4[0] -> page_table_l3_identity
    ; PDPT[0] -> page_table_l2_identity
    ;
    ; 512 x 2 MiB = first 1 GiB
    ; --------------------------------------------------------

    mov eax, page_table_l3_identity
    or eax, 0b11
    mov [page_table_l4 + 0 * 8], eax

    mov eax, page_table_l2_identity
    or eax, 0b11
    mov [page_table_l3_identity + 0 * 8], eax

    xor ecx, ecx

.identity_loop:
    mov eax, 0x200000
    mul ecx

    ; Present + writable + 2 MiB page
    or eax, 0b10000011

    mov [page_table_l2_identity + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .identity_loop


    ; --------------------------------------------------------
    ; Higher-half mapping
    ;
    ; PML4[511] -> page_table_l3_high
    ; PDPT[0]   -> page_table_l2_high
    ;
    ; Virtual:
    ;   0xFFFFFFFF80000000
    ;
    ; Physical:
    ;   0x00000000
    ;
    ; 512 x 2 MiB = first 1 GiB
    ; --------------------------------------------------------

    mov eax, page_table_l3_high
    or eax, 0b11
    mov [page_table_l4 + 511 * 8], eax

    mov eax, page_table_l2_high
    or eax, 0b11
    mov [page_table_l3_high + 510 * 8], eax

    xor ecx, ecx

.high_loop:
    mov eax, 0x200000
    mul ecx

    ; Present + writable + 2 MiB page
    or eax, 0b10000011

    mov [page_table_l2_high + ecx * 8], eax

    inc ecx
    cmp ecx, 512
    jne .high_loop

    ret


enable_page_table:
    ; Load PML4
    mov eax, page_table_l4
    mov cr3, eax

    ; Enable PAE
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax

    ; Enable Long Mode through EFER.LME
    mov ecx, 0xC0000080
    rdmsr

    or eax, 1 << 8
    wrmsr

    ; Enable paging
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax

    ret


enable_nxe:
    mov ecx, 0xC0000080
    rdmsr

    or eax, 1 << 11

    wrmsr

    ret


enable_write_protect:
    mov eax, cr0
    or eax, 1 << 16

    mov cr0, eax

    ret


error:
    mov dword [0xb8000], 0x4f524f45
    mov dword [0xb8004], 0x4f324f52
    mov dword [0xb8008], 0x4f204f20
    mov byte [0xb800a], al

    cli

.hang:
    hlt
    jmp .hang


; ------------------------------------------------------------
; Bootstrap data
;
; These objects MUST remain in low physical memory because
; 32-bit bootstrap code accesses them before the higher-half
; transition.
; ------------------------------------------------------------

section .boot.bss nobits

alignb 4096

global page_table_l4

page_table_l4:
    resb 4096

page_table_l3_identity:
    resb 4096

page_table_l2_identity:
    resb 4096

page_table_l3_high:
    resb 4096

page_table_l2_high:
    resb 4096

multiboot_info:
    resd 1

alignb 16

start_stack:
    resb 4096 * 4

end_stack: