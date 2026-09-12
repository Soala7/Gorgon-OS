# Gorgon VOS Phase 2 Roadmap

This document is the master plan for the real operating-system phase of Gorgon.

## Scope

Gorgon VOS is being built as a native x86-64 hybrid kernel with a native userspace and a Linux guest virtualization layer. The roadmap intentionally prioritizes correctness, bootability, and debugability before higher-level UI and virtualization work.

## Phase structure

- Phase 2A: Kernel Foundation
  - M1: Build, boot, and debug foundation
  - M2: CPU architecture and basic interrupts
  - M3: Physical memory
  - M4: Virtual memory
  - M5: Kernel heap
  - M6: Processes
  - M7: Threads
  - M8: Scheduler
  - M9: System calls
  - M10: IPC
- Phase 2B: Userspace + Core OS
- Phase 2C: Virtualization
- Phase 2D: Linux integration
- Phase 2E: Desktop + native experience
- Phase 2F: Polish + ecosystem

## Current status

Status: Phase 2A — Kernel Foundation

Current milestone: M1 — Build + GRUB + Multiboot2 + Serial + QEMU + GDB

## Immediate target

The M1 milestone focuses on establishing a working engineering loop:

1. Source builds cleanly
2. Cross-toolchain and linker produce a kernel binary
3. GRUB loads the kernel from a bootable ISO
4. Multiboot2 entry works
5. Early CPU initialization succeeds
6. Serial output is available on COM1
7. QEMU + GDB can inspect the booted kernel

## Architectural principles

- Gorgon is the host operating system.
- Linux is a guest environment.
- Kernel mechanisms stay in the kernel; policy and services stay in userspace when practical.
- Debugging is a first-class requirement from day one.
- Correctness and stability come before optimization.

## Long-term goal

Build a native Gorgon operating system with its own runtime, userspace, desktop, SDK, and virtualization infrastructure, while using a Linux guest environment for application compatibility where appropriate.
