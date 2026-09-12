global idt_load
global exception_handler
global interrupt_handler
global timer_interrupt

extern timer_tick
extern pic_send_eoi

extern serial_write_str
extern serial_write_hex

section .text
bits 64

idt_load:
xor ecx, ecx

.fill_generic:
mov rax, interrupt_stub_generic
mov rdx, idt
lea rdx, [rdx + rcx * 8]
lea rdx, [rdx + rcx * 8]

mov word [rdx + 0], ax
shr rax, 16
mov word [rdx + 6], ax
mov word [rdx + 2], 0x08
mov byte [rdx + 4], 0
mov byte [rdx + 5], 0x8E
shr rax, 16
mov dword [rdx + 8], eax
mov dword [rdx + 12], 0

inc ecx
cmp ecx, 256
jne .fill_generic

mov ecx, 0

.exception_loop:
mov rax, exception_stub_table
mov rax, [rax + rcx * 8]

mov rdx, idt
lea rdx, [rdx + rcx * 8]
lea rdx, [rdx + rcx * 8]

mov word [rdx + 0], ax
shr rax, 16
mov word [rdx + 6], ax
mov word [rdx + 2], 0x08
mov byte [rdx + 4], 0
mov byte [rdx + 5], 0x8E
shr rax, 16
mov dword [rdx + 8], eax
mov dword [rdx + 12], 0

inc ecx
cmp ecx, 32
jne .exception_loop

mov rax, interrupt_stub_32

mov rdx, idt
add rdx, 32 * 16

mov word [rdx + 0], ax
shr rax, 16
mov word [rdx + 6], ax
mov word [rdx + 2], 0x08
mov byte [rdx + 4], 0
mov byte [rdx + 5], 0x8E
shr rax, 16
mov dword [rdx + 8], eax
mov dword [rdx + 12], 0

lidt [idt_pointer]
ret

timer_interrupt:
    cli

    push rax
    push rbx
    push rcx
    push rdx
    push rsi
    push rdi
    push rbp
    push r8
    push r9
    push r10
    push r11
    push r12
    push r13
    push r14
    push r15

    sub rsp, 8

    lea rdi, [rsp + 8]

    call timer_tick

    mov r12, rax

    test r12, r12
    jz .send_eoi

    lea rdi, [rsp + 8]

    ; task_t layout:
    ; id       = 0
    ; state    = 8
    ; padding  = 12
    ; stack    = 16
    ; context  = 16400
    ;
    ; 16 + 16384 = 16400

    lea rsi, [r12 + 16400]

    mov rax, [rsi + 0]
    mov [rdi + 0], rax

    mov rax, [rsi + 8]
    mov [rdi + 8], rax

    mov rax, [rsi + 16]
    mov [rdi + 16], rax

    mov rax, [rsi + 24]
    mov [rdi + 24], rax

    mov rax, [rsi + 32]
    mov [rdi + 32], rax

    mov rax, [rsi + 40]
    mov [rdi + 40], rax

    mov rax, [rsi + 48]
    mov [rdi + 48], rax

    mov rax, [rsi + 56]
    mov [rdi + 56], rax

    mov rax, [rsi + 64]
    mov [rdi + 64], rax

    mov rax, [rsi + 72]
    mov [rdi + 72], rax

    mov rax, [rsi + 80]
    mov [rdi + 80], rax

    mov rax, [rsi + 88]
    mov [rdi + 88], rax

    mov rax, [rsi + 96]
    mov [rdi + 96], rax

    mov rax, [rsi + 104]
    mov [rdi + 104], rax

    mov rax, [rsi + 112]
    mov [rdi + 112], rax

    mov rax, [rsi + 120]
    mov [rdi + 128], rax

    mov rax, [rsi + 128]
    mov [rdi + 136], rax

    mov rax, [rsi + 136]
    mov [rdi + 144], rax

.send_eoi:

    mov al, 0
    call pic_send_eoi

    add rsp, 8

    pop r15
    pop r14
    pop r13
    pop r12
    pop r11
    pop r10
    pop r9
    pop r8
    pop rbp
    pop rdi
    pop rsi
    pop rdx
    pop rcx
    pop rbx
    pop rax

    add rsp, 8

    iretq

interrupt_handler:
cli

push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push rbp
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15

.hang:
hlt
jmp .hang

exception_handler:
cli

push rax
push rbx
push rcx
push rdx
push rsi
push rdi
push rbp
push r8
push r9
push r10
push r11
push r12
push r13
push r14
push r15

mov rdi, [rsp + 120]
lea rsi, [rel exception_vector_message]
call serial_write_str

mov rdi, [rsp + 120]
call serial_write_hex

lea rdi, [rel exception_error_message]
call serial_write_str

mov rdi, [rsp + 128]
call serial_write_hex

lea rdi, [rel newline_message]
call serial_write_str

.exception_hang:
hlt
jmp .exception_hang

%macro EXCEPTION_STUB 1
exception_stub_%1:
push %1
jmp exception_handler
%endmacro

interrupt_stub_32:
push 32
jmp timer_interrupt

interrupt_stub_generic:
push 255
jmp interrupt_handler

EXCEPTION_STUB 0
EXCEPTION_STUB 1
EXCEPTION_STUB 2
EXCEPTION_STUB 3
EXCEPTION_STUB 4
EXCEPTION_STUB 5
EXCEPTION_STUB 6
EXCEPTION_STUB 7
EXCEPTION_STUB 8
EXCEPTION_STUB 9
EXCEPTION_STUB 10
EXCEPTION_STUB 11
EXCEPTION_STUB 12
EXCEPTION_STUB 13
EXCEPTION_STUB 14
EXCEPTION_STUB 15
EXCEPTION_STUB 16
EXCEPTION_STUB 17
EXCEPTION_STUB 18
EXCEPTION_STUB 19
EXCEPTION_STUB 20
EXCEPTION_STUB 21
EXCEPTION_STUB 22
EXCEPTION_STUB 23
EXCEPTION_STUB 24
EXCEPTION_STUB 25
EXCEPTION_STUB 26
EXCEPTION_STUB 27
EXCEPTION_STUB 28
EXCEPTION_STUB 29
EXCEPTION_STUB 30
EXCEPTION_STUB 31

section .rodata

exception_stub_table:
dq exception_stub_0
dq exception_stub_1
dq exception_stub_2
dq exception_stub_3
dq exception_stub_4
dq exception_stub_5
dq exception_stub_6
dq exception_stub_7
dq exception_stub_8
dq exception_stub_9
dq exception_stub_10
dq exception_stub_11
dq exception_stub_12
dq exception_stub_13
dq exception_stub_14
dq exception_stub_15
dq exception_stub_16
dq exception_stub_17
dq exception_stub_18
dq exception_stub_19
dq exception_stub_20
dq exception_stub_21
dq exception_stub_22
dq exception_stub_23
dq exception_stub_24
dq exception_stub_25
dq exception_stub_26
dq exception_stub_27
dq exception_stub_28
dq exception_stub_29
dq exception_stub_30
dq exception_stub_31

section .bss

align 16

idt:
resb 4096

section .data

idt_pointer:
dw 4095
dq idt

section .rodata

exception_vector_message:
db "Exception vector: 0x", 0

exception_error_message:
db " Error code: 0x", 0

newline_message:
db 10, 0
