# Serial Port Reference: COM1

This document records the hardware-level serial register layout used during Gorgon’s early kernel debug work.

## COM1 base address

- `0x3F8`

## UART register map

| Offset | Register | Purpose |
| --- | --- | --- |
| `0x00` | Data Register | Transmit / Receive data |
| `0x01` | Interrupt Enable | Enables UART interrupts |
| `0x02` | Interrupt ID / FIFO Control | Interrupt status and FO control |
| `0x03` | Line Control | Set baud, parity, and data bits |
| `0x04` | Modem Control | RTS / DTR / loopback control |
| `0x05` | Line Status | Tx ready / Rx ready status |
| `0x06` | Modem Status | modem state |
| `0x07` | Scratch Register | optional debug/test register |

## Typical initialization sequence

For a standard 16550-compatible UART, the early initialization pattern is:

```c
outb(COM1 + 3, 0x80); // DLAB = 1
outb(COM1 + 0, 0x01); // baud divisor low
outb(COM1 + 1, 0x00); // baud divisor high
outb(COM1 + 3, 0x03); // 8N1
outb(COM1 + 2, 0xC7); // enable FIFO
outb(COM1 + 4, 0x0B); // enable IRQs / modem control
```

## Line status bit

The transmit-ready check is usually done with bit 5 of the line status register:

- `0x20` = transmitter empty

Example:

```c
while ((inb(COM1 + 5) & 0x20) == 0) {
    // wait until UART is ready
}
```

## Notes

This is primarily used for early boot and kernel debugging, not as a final UI or system console abstraction.
