| File                                | What we will do                                                                                           |
| ----------------------------------- | --------------------------------------------------------------------------------------------------------- |
| `src/x86_64/cpu/pic.c`              | Fix PIC masks to `0xFE / 0xFF`; keep only IRQ0 enabled                                                    |
| `src/x86_64/cpu/pic.asm`            | Fix `pic_send_eoi` to use the C ABI (`RDI`)                                                               |
| `src/x86_64/cpu/pic.h`              | Verify EOI declaration matches implementation                                                             |
| `src/x86_64/cpu/serial.c`           | Disable unnecessary UART interrupts; remove duplicate I/O helpers                                         |
| `src/x86_64/cpu/io.h`               | Keep the single canonical `inb/outb` implementation                                                       |
| `src/x86_64/cpu/idt.asm`            | Fix exception frames/error-code handling; fix exception printing; prepare proper interrupt-frame handling |
| `src/x86_64/cpu/gdt.asm`            | Extend GDT for TSS support                                                                                |
| `src/x86_64/cpu/gdt.h`              | Add TSS/GDT declarations if required                                                                      |
| `src/x86_64/cpu/tss.asm` or `tss.c` | Add TSS and dedicated fault/interrupt stack                                                               |
| `src/x86_64/cpu/tss.h`              | TSS interface                                                                                             |
| `src/x86_64/cpu/task.h`             | Consolidate context structures and remove the duplicated layout                                           |
| `src/x86_64/task.c`                 | Later: integrate tasks with real stacks/PMM; **scheduler stays disabled for now**                         |
| `src/x86_64/cpu/context.h`          | Remove/replace duplicate `cpu_context_t`                                                                  |
| `src/x86_64/cpu/context_switch.asm` | Later rewrite when real context switching begins                                                          |
| `src/x86_64/cpu/timer.c`            | Keep timer ticking; don't perform task switching yet                                                      |
| `src/x86_64/cpu/timer.h`            | Keep the correct `timer_tick()` interface                                                                 |
| `src/memory/pmm.c`                  | Reserve kernel/bitmap/page tables/boot stack/Multiboot info; fix allocation semantics                     |
| `src/memory/pmm.h`                  | Update PMM API                                                                                            |
| `src/x86_64/boot/main.asm`          | Already fixed; only extend if PMM needs additional boot information                                       |
| `targets/x86_64/linker.ld`          | Already fixed; expose/use `kernel_start/kernel_end` correctly                                             |
| `Makefile`                          | Already improved; add any new TSS/PMM objects                                                             |
