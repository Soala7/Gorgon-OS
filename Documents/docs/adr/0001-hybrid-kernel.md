# ADR 0001: Hybrid Kernel Architecture

- Status: Accepted

## Context

Gorgon is being built as a real operating system, but a strict microkernel design would not match the practical goals of the project. The system must eventually include core CPU, memory, scheduling, and virtualization responsibilities in the kernel while keeping higher-level policy and services in userspace.

## Decision

Use a hybrid kernel architecture.

The kernel will contain the low-level mechanisms required for reliable operation:

- CPU management
- memory management
- virtualization primitives
- scheduling
- IPC primitives
- syscalls
- low-level device support

Userspace will own the policy-heavy subsystems, including:

- filesystem services
- device management
- desktop components
- user applications
- VM management
- service orchestration

## Consequences

### Positive

- Simpler implementation path for the early kernel
- Easier debugging and control during early boot stages
- Better fit for a real x86-64 development timeline

### Negative

- Not as minimal as a traditional microkernel
- Requires careful kernel/userspace separation decisions as the project grows

## Notes

This architecture aligns with Gorgon’s long-term goal of a native host operating system with Linux guest compatibility, rather than trying to reproduce a Linux-like monolithic design in the early stages.
