# ADR 0003: Use COM1 Serial Output for Early Debugging

- Status: Accepted

## Context

The kernel will be developed in bare-metal environments and virtualized QEMU sessions. A reliable, low-level debug mechanism is required from the earliest possible stage.

## Decision

Use COM1 as the primary early debug channel.

The standard UART port mapping for COM1 is:

- Base port: `0x3F8`
- Transmit buffer: `0x3F8`
- Interrupt enable: `0x3F9`
- FIFO control: `0x3FA`
- Line control: `0x3FB`
- Modem control: `0x3FC`
- Line status: `0x3FD`

## Consequences

### Positive

- Works well in QEMU and real hardware
- Simple to implement in a freestanding kernel
- Allows debugging before a full console subsystem exists
- Enables early boot validation and fault reporting

### Negative

- Not a user-facing display system
- Not sufficient for full kernel UI or later runtime diagnostics

## Notes

Serial output is the de facto standard for early x86 boot and kernel debugging. It provides the minimum required observability while the kernel is still too small to support a richer debugging UI.
