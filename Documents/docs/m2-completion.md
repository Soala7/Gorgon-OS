# M2 Plan: CPU Architecture and Early Interrupts

## Goal

M2 expands the boot foundation into a proper x86-64 CPU initialization and interrupt framework.

## Core tasks

### 1. CPU startup hardening

- verify long-mode startup and CPU state
- ensure the GDT and segment setup are stable
- validate the transition from 32-bit protected mode to 64-bit mode

### 2. Interrupt Descriptor Table (IDT)

- create the IDT structure
- populate exception handlers
- configure interrupt gates
- load the IDT with `lidt`

### 3. Exception handling

Implement handlers for:

- divide by zero
- invalid opcode
- general protection fault
- page fault

These must report state through serial output and halt cleanly.

### 4. IRQ routing

- map PIC or APIC initialization paths as needed
- establish a stable interrupt flow for future timers and device work
- keep the design simple and testable

### 5. Controlled failure testing

The kernel should deliberately test the following conditions:

- divide by zero
- invalid opcode
- general protection fault
- page fault

This confirms that exception reporting and serial logging work as expected.

## Deliverables

By the end of M2, the project should have:

- reliable CPU startup sequence
- working IDT setup
- early exception handlers
- serial-visible fault diagnostics
- a stable debug baseline for the next memory milestones

## Success criteria

- QEMU boots the kernel without startup failure
- faults trigger the expected exception path
- serial output logs the fault reason and CPU state
- the system remains controllable enough for later debug sessions
