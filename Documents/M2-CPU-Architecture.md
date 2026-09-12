# Gorgon OS — M2: CPU Architecture & Exception Handling

## Objective

Establish the core x86-64 CPU architecture and exception-handling infrastructure required for Gorgon to safely receive, identify, and handle processor exceptions and hardware interrupts.

**Architecture:** x86-64

**CPU mode:** 64-bit Long Mode

**Language:** C / x86-64 Assembly

**Interrupt controller:** 8259 PIC

**Timer:** Intel 8253/8254-compatible PIT

**Debug interface:** COM1 Serial

**Virtual hardware:** QEMU

---

# 1. CPU Execution Environment

Gorgon enters x86-64 long mode during the early boot process.

The CPU startup path is:

```text
GRUB
 ↓
32-bit protected mode
 ↓
CPU capability checks
 ↓
Page tables
 ↓
Enable PAE
 ↓
Enable long mode
 ↓
Enable paging
 ↓
Load GDT
 ↓
Far jump
 ↓
64-bit long mode
 ↓
Load IDT
 ↓
Kernel execution
```

This provides the CPU environment required for the kernel's 64-bit execution.

---

# 2. Global Descriptor Table

Gorgon uses a Global Descriptor Table to define the code and data segments required during the transition into 64-bit execution.

The current GDT contains:

```text
Entry 0 → Null descriptor
Entry 1 → 64-bit code segment
Entry 2 → Data segment
```

The selectors are:

```text
Code segment → 0x08
Data segment → 0x10
```

The GDT is loaded using:

```asm
lgdt [gdt_pointer]
```

Gorgon then performs a far jump to the 64-bit code segment:

```asm
jmp 0x08:long_mode_start
```

This establishes the correct code segment before executing the 64-bit kernel path.

---

# 3. Interrupt Descriptor Table

Gorgon implements a 64-bit Interrupt Descriptor Table containing space for all 256 x86 interrupt vectors.

Each 64-bit IDT entry occupies:

```text
16 bytes
```

The complete table therefore requires:

```text
256 × 16 = 4096 bytes
```

The IDT is loaded after entering long mode:

```asm
call idt_load
```

followed by:

```asm
lidt [idt_pointer]
```

Each IDT gate currently uses the kernel code segment:

```text
0x08
```

with an interrupt gate type.

---

# 4. CPU Exception Framework

Gorgon now provides individual exception stubs for the first 32 processor exception vectors.

The exception vectors cover the x86 exception range:

```text
0   Divide Error
1   Debug
2   Non-Maskable Interrupt
3   Breakpoint
4   Overflow
5   Bound Range Exceeded
6   Invalid Opcode
7   Device Not Available
8   Double Fault
9   Coprocessor Segment Overrun
10  Invalid TSS
11  Segment Not Present
12  Stack-Segment Fault
13  General Protection Fault
14  Page Fault
15  Reserved
16  x87 Floating-Point Exception
17  Alignment Check
18  Machine Check
19  SIMD Floating-Point Exception
20  Virtualization Exception
21  Control Protection Exception
22  Reserved
23  Reserved
24  Reserved
25  Reserved
26  Reserved
27  Reserved
28  Hypervisor Injection Exception
29  VMM Communication Exception
30  Security Exception
31  Reserved
```

Each exception receives its vector number through an assembly stub before entering the common exception handler.

The general structure is:

```text
CPU Exception
 ↓
Exception Stub
 ↓
Push Exception Vector
 ↓
Common Exception Handler
 ↓
Serial Diagnostic Output
 ↓
CPU Halt
```

---

# 5. Exception Diagnostics

Gorgon provides early exception diagnostics through the COM1 serial interface.

The exception handler reports:

```text
Exception vector: 0x...
Error code: 0x...
```

This allows CPU faults to be identified directly from QEMU's serial output.

For example, a General Protection Fault can be identified as:

```text
Exception vector: 0x000000000000000D
Error code: 0x...
```

This is particularly important during kernel development because an exception that is not handled correctly can otherwise appear simply as a system reboot or triple fault.

---

# 6. Interrupt Context

Gorgon defines an interrupt context structure for saving the processor state during interrupts.

The saved general-purpose registers include:

```text
R15
R14
R13
R12
R11
R10
R9
R8
RBP
RDI
RSI
RDX
RCX
RBX
RAX
```

The interrupt frame also contains:

```text
Vector
RIP
CS
RFLAGS
```

This provides the scheduler and interrupt subsystem with access to the CPU state that was active when an interrupt occurred.

---

# 7. Hardware Interrupt Infrastructure

Gorgon now connects the CPU interrupt system to the legacy x86 Programmable Interrupt Controller.

The PIC is remapped so hardware IRQs do not conflict with CPU exception vectors.

The mapping is:

```text
Master PIC

IRQ0 → 0x20
IRQ1 → 0x21
IRQ2 → 0x22
IRQ3 → 0x23
IRQ4 → 0x24
IRQ5 → 0x25
IRQ6 → 0x26
IRQ7 → 0x27


Slave PIC

IRQ8  → 0x28
IRQ9  → 0x29
IRQ10 → 0x2A
IRQ11 → 0x2B
IRQ12 → 0x2C
IRQ13 → 0x2D
IRQ14 → 0x2E
IRQ15 → 0x2F
```

This separates hardware interrupts from the processor's exception vectors.

---

# 8. Programmable Interval Timer

Gorgon initializes the PIT as the first hardware timer source.

The PIT is configured in Mode 3 using a divisor of:

```text
11931
```

This produces approximately:

```text
100 timer interrupts per second
```

The timer interrupt is therefore generated approximately every:

```text
10 ms
```

The timer is connected to:

```text
IRQ0
```

which is mapped to:

```text
Interrupt vector 0x20
```

---

# 9. Timer Interrupt Path

The complete timer interrupt path is now:

```text
PIT
 ↓
IRQ0
 ↓
PIC
 ↓
Vector 0x20
 ↓
Timer Interrupt Stub
 ↓
Save CPU Registers
 ↓
timer_tick()
 ↓
Kernel Timer / Scheduler
 ↓
Send PIC EOI
 ↓
Restore CPU Registers
 ↓
iretq
```

The timer subsystem maintains a global tick counter:

```c
volatile uint64_t timer_ticks;
```

Each successful timer interrupt increments this counter.

---

# 10. PIC End-of-Interrupt

After processing a hardware interrupt, Gorgon sends an End-of-Interrupt command to the PIC.

For IRQ0, the master PIC receives:

```text
0x20
```

For future slave-PIC interrupts, the slave PIC is acknowledged first, followed by the master PIC.

This allows the PIC to continue delivering subsequent hardware interrupts.

---

# 11. Interrupt Enabling

Hardware interrupts remain disabled during the early CPU and kernel initialization stages.

After the PIC and PIT have been configured, Gorgon enables interrupts with:

```asm
sti
```

The kernel can then receive timer interrupts while executing normally.

The kernel uses:

```asm
hlt
```

while idle, allowing the timer interrupt to wake the processor.

---

# 12. Generic Interrupt Infrastructure

Gorgon provides a generic interrupt stub for interrupt vectors that do not yet have a dedicated device handler.

The current generic path is:

```text
Interrupt
 ↓
Generic Interrupt Stub
 ↓
Generic Interrupt Handler
 ↓
CPU Halt
```

This prevents unimplemented interrupts from silently executing unknown code.

Dedicated device handlers can replace these generic handlers as additional hardware support is implemented.

---

# 13. Verified CPU/Interrupt Behavior

The following behavior has been successfully tested in QEMU:

```text
Kernel starts
 ↓
PIC initialized
 ↓
PIT initialized
 ↓
Interrupts enabled
 ↓
Timer IRQs received
 ↓
Timer tick counter increments
 ↓
Kernel continues executing
```

The system remains running instead of immediately rebooting or triple faulting.

---

# 14. M2 Verification Checklist

| Component                  | Status |
| -------------------------- | ------ |
| x86-64 long-mode execution | ✓      |
| GDT                        | ✓      |
| 64-bit code segment        | ✓      |
| 64-bit data segment        | ✓      |
| IDT creation               | ✓      |
| 256 interrupt entries      | ✓      |
| IDT loading                | ✓      |
| CPU exception stubs        | ✓      |
| Common exception handler   | ✓      |
| Exception vector reporting | ✓      |
| Error-code reporting       | ✓      |
| Interrupt context          | ✓      |
| PIC remapping              | ✓      |
| IRQ0 → vector 0x20         | ✓      |
| PIT initialization         | ✓      |
| ~100 Hz timer              | ✓      |
| Timer interrupt handler    | ✓      |
| PIC EOI                    | ✓      |
| Generic interrupt handler  | ✓      |
| `sti` interrupt enabling   | ✓      |
| `hlt` kernel idle path     | ✓      |
| Timer ticks verified       | ✓      |

---

# M2 Result

**M2 — CPU Architecture & Exception Handling: COMPLETE**

Gorgon now has a functional x86-64 CPU interrupt foundation capable of:

* Executing in 64-bit long mode.
* Loading and using the GDT.
* Loading a complete 256-entry IDT.
* Receiving processor exceptions.
* Identifying exception vectors.
* Reporting exception information through COM1.
* Receiving hardware timer interrupts.
* Remapping the legacy PIC.
* Configuring the PIT at approximately 100 Hz.
* Saving CPU state during interrupts.
* Returning from interrupts with `iretq`.
* Maintaining a kernel timer tick counter.

This establishes the CPU and interrupt foundation required for the next stage of Gorgon's kernel development.

**M2 is done.**
