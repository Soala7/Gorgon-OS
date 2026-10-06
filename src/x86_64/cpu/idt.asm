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

; ------------------------------------------------------------
; IDT initialization
; ------------------------------------------------------------

idt_load:
    ; Fill vectors 33-255 with per-vector generic interrupt stubs.
    ;
    ; Vectors 0-31 are installed separately as CPU exception
    ; handlers below. Vector 32 is installed as the PIT timer
    ; interrupt after the exception handlers are configured.
    xor ecx, ecx

.fill_generic:
    mov rax, [generic_stub_table + rcx * 8]
    mov rdx, idt

    ; IDT entry index = vector number.
    ; Each IDT entry is 16 bytes.
    lea rdx, [rdx + rcx * 8]
    lea rdx, [rdx + rcx * 8]
    add rdx, 33 * 16

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
    cmp ecx, 256 - 33
    jne .fill_generic


    ; --------------------------------------------------------
    ; Install exception handlers for vectors 0-31.
    ; --------------------------------------------------------

    xor ecx, ecx

.exception_loop:
    mov rax, exception_stub_table
    mov rax, [rax + rcx * 8]

    mov rdx, idt

    ; Each IDT entry is 16 bytes.
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


    ; --------------------------------------------------------
    ; Double Fault (#DF = vector 8)
    ; --------------------------------------------------------
    ;
    ; Vector 8 uses IST1 so that a double fault receives a
    ; dedicated stack supplied by the TSS.
    ;
    ; tss_init() must have loaded the TSS with LTR before an
    ; interrupt using IST1 can safely occur.
    ;

    mov rdx, idt
    add rdx, 8 * 16
    mov byte [rdx + 4], 1


    ; --------------------------------------------------------
    ; IRQ0 -> interrupt vector 32
    ; --------------------------------------------------------

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


; ------------------------------------------------------------
; Timer interrupt
; ------------------------------------------------------------

timer_interrupt:
    cli

    ; Save all general-purpose registers.
    ;
    ; After these 15 pushes, RSP points to the beginning of
    ; interrupt_context_t:
    ;
    ; +0    r15
    ; +8    r14
    ; +16   r13
    ; +24   r12
    ; +32   r11
    ; +40   r10
    ; +48   r9
    ; +56   r8
    ; +64   rbp
    ; +72   rdi
    ; +80   rsi
    ; +88   rdx
    ; +96   rcx
    ; +104  rbx
    ; +112  rax
    ; +120  vector
    ; +128  RIP
    ; +136  CS
    ; +144  RFLAGS
    ;
    ; The CPU does not push an error code for IRQ0.

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

    ; RSP now points directly to interrupt_context_t.
    mov rdi, rsp
    call timer_tick

    xor edi, edi
    call pic_send_eoi


    ; --------------------------------------------------------
    ; Restore registers
    ; --------------------------------------------------------

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

    ; Remove the vector pushed by interrupt_stub_32.
    add rsp, 8

    iretq


; ------------------------------------------------------------
; Generic interrupt handler
; ------------------------------------------------------------

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

    lea rdi, [rel interrupt_vector_message]
    call serial_write_str

    mov rdi, [rsp + 120]
    call serial_write_hex

    lea rdi, [rel newline_message]
    call serial_write_str


.generic_interrupt_hang:
    hlt
    jmp .generic_interrupt_hang


; ------------------------------------------------------------
; Exception handler
; ------------------------------------------------------------

exception_handler:
    cli

    ; Save all general-purpose registers.
    ;
    ; After these 15 pushes, the normalized exception frame is:
    ;
    ; +0    r15
    ; +8    r14
    ; +16   r13
    ; +24   r12
    ; +32   r11
    ; +40   r10
    ; +48   r9
    ; +56   r8
    ; +64   rbp
    ; +72   rdi
    ; +80   rsi
    ; +88   rdx
    ; +96   rcx
    ; +104  rbx
    ; +112  rax
    ; +120  exception vector
    ; +128  error code
    ; +136  RIP
    ; +144  CS
    ; +152  RFLAGS
    ;
    ; For exceptions where the CPU does not push an error code,
    ; the stub pushes a synthetic zero error code.
    ;
    ; For exceptions where the CPU does push an error code,
    ; the stub leaves that CPU-pushed value in the same position.

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


    ; --------------------------------------------------------
    ; Check for Page Fault (#PF = vector 14)
    ; --------------------------------------------------------

    cmp qword [rsp + 120], 14
    je page_fault_handler


    ; --------------------------------------------------------
    ; Generic exception reporting
    ; --------------------------------------------------------

    lea rdi, [rel exception_vector_message]
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


; ------------------------------------------------------------
; Page Fault Handler
; ------------------------------------------------------------

page_fault_handler:

    ; CR2 contains the virtual address that caused the page fault.
    ; Read it immediately before calling any other code.

    mov rax, cr2

    push rax

    lea rdi, [rel page_fault_message]
    call serial_write_str

    lea rdi, [rel page_fault_address_message]
    call serial_write_str

    mov rdi, [rsp]
    call serial_write_hex

    lea rdi, [rel page_fault_error_message]
    call serial_write_str

    mov rdi, [rsp + 136]
    call serial_write_hex

    lea rdi, [rel newline_message]
    call serial_write_str

    ; Remove saved CR2.
    add rsp, 8


.page_fault_hang:
    hlt
    jmp .page_fault_hang


; ------------------------------------------------------------
; Exception stubs
; ------------------------------------------------------------

; Exceptions where the CPU does NOT automatically push an
; error code.
;
; Push a synthetic zero error code so that every exception
; reaches exception_handler with the same normalized layout:
;
; [rsp + 120] = vector
; [rsp + 128] = error code

%macro EXCEPTION_NO_ERROR 1
exception_stub_%1:
    push qword 0
    push qword %1
    jmp exception_handler
%endmacro


; Exceptions where the CPU automatically pushes an error code.
;
; The CPU's error code is already present, so only the vector
; is pushed here.
;
; After entering exception_handler:
;
; [rsp + 120] = vector
; [rsp + 128] = CPU-pushed error code

%macro EXCEPTION_ERROR 1
exception_stub_%1:
    push qword %1
    jmp exception_handler
%endmacro


; ------------------------------------------------------------
; IRQ stubs
; ------------------------------------------------------------

interrupt_stub_32:
    push qword 32
    jmp timer_interrupt


; ------------------------------------------------------------
; Per-vector generic interrupt stubs
; Vectors 33-255
; ------------------------------------------------------------

%assign i 33
%rep (256 - 33)

interrupt_stub_%+i:
    push qword i
    jmp interrupt_handler

%assign i i + 1
%endrep


; ------------------------------------------------------------
; CPU exceptions
; ------------------------------------------------------------

EXCEPTION_NO_ERROR 0      ; #DE Divide Error
EXCEPTION_NO_ERROR 1      ; #DB Debug
EXCEPTION_NO_ERROR 2      ; NMI
EXCEPTION_NO_ERROR 3      ; #BP Breakpoint
EXCEPTION_NO_ERROR 4      ; #OF Overflow
EXCEPTION_NO_ERROR 5      ; #BR BOUND Range Exceeded
EXCEPTION_NO_ERROR 6      ; #UD Invalid Opcode
EXCEPTION_NO_ERROR 7      ; #NM Device Not Available

EXCEPTION_ERROR    8      ; #DF Double Fault

EXCEPTION_NO_ERROR 9      ; Coprocessor Segment Overrun

EXCEPTION_ERROR    10     ; #TS Invalid TSS
EXCEPTION_ERROR    11     ; #NP Segment Not Present
EXCEPTION_ERROR    12     ; #SS Stack-Segment Fault
EXCEPTION_ERROR    13     ; #GP General Protection Fault
EXCEPTION_ERROR    14     ; #PF Page Fault

EXCEPTION_NO_ERROR 15     ; Reserved
EXCEPTION_NO_ERROR 16     ; #MF x87 Floating-Point Exception

EXCEPTION_ERROR    17     ; #AC Alignment Check

EXCEPTION_NO_ERROR 18     ; #MC Machine Check
EXCEPTION_NO_ERROR 19     ; #XM SIMD Floating-Point Exception
EXCEPTION_NO_ERROR 20     ; #VE Virtualization Exception

EXCEPTION_ERROR    21     ; #CP Control Protection Exception

EXCEPTION_NO_ERROR 22     ; Reserved
EXCEPTION_NO_ERROR 23     ; Reserved
EXCEPTION_NO_ERROR 24     ; Reserved
EXCEPTION_NO_ERROR 25     ; Reserved
EXCEPTION_NO_ERROR 26     ; Reserved
EXCEPTION_NO_ERROR 27     ; Reserved
EXCEPTION_NO_ERROR 28     ; #HV Hypervisor Injection Exception

EXCEPTION_ERROR    29     ; #VC VMM Communication Exception
EXCEPTION_ERROR    30     ; #SX Security Exception

EXCEPTION_NO_ERROR 31     ; Reserved


; ------------------------------------------------------------
; Exception stub table
; ------------------------------------------------------------

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


; ------------------------------------------------------------
; Generic interrupt stub table
; Vectors 33-255
; ------------------------------------------------------------

generic_stub_table:
%assign i 33
%rep (256 - 33)
    dq interrupt_stub_%+i
%assign i i + 1
%endrep


; ------------------------------------------------------------
; IDT storage
; ------------------------------------------------------------

section .bss

align 16

idt:
    resb 4096


; ------------------------------------------------------------
; IDT pointer
; ------------------------------------------------------------

section .data

idt_pointer:
    dw 4095
    dq idt


; ------------------------------------------------------------
; Diagnostic messages
; ------------------------------------------------------------

section .rodata

exception_vector_message:
    db "Exception vector: 0x", 0

exception_error_message:
    db " Error code: 0x", 0

interrupt_vector_message:
    db "Unhandled interrupt vector: 0x", 0

page_fault_message:
    db "Page Fault", 10, 0

page_fault_address_message:
    db "Fault address: 0x", 0

page_fault_error_message:
    db " Error code: 0x", 0

newline_message:
    db 10, 0