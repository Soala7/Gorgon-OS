| #  | Step                                | Goal                                                                           | Main files                                  |
| -- | ----------------------------------- | ------------------------------------------------------------------------------ | ------------------------------------------- |
| 1  | **Boot stack fix**                  | Make the boot stack grow into its reserved space correctly                     | `src/x86_64/boot/main.asm`                  |
| 2  | **Linker sections**                 | Properly include `.text`, `.rodata`, `.data`, `.bss`, kernel stack/page tables | `targets/x86_64/linker.ld`                  |
| 3  | **Compiler flags**                  | Make kernel C safe for interrupt-driven execution                              | `Makefile`                                  |
| 4  | **Save Multiboot2 info**            | Preserve `EBX` so Gorgon can read the memory map                               | `src/x86_64/boot/main.asm`, boot C          |
| 5  | **PIC cleanup**                     | Mask unused IRQs, especially keyboard IRQ1                                     | `src/x86_64/cpu/pic.c/.h`                   |
| 6  | **Exception frames**                | Correctly handle exceptions with/without CPU error codes                       | `src/x86_64/cpu/idt.asm`                    |
| 7  | **TSS / fault stack**               | Give serious CPU faults a safe stack                                           | `src/x86_64/cpu/gdt.*`, new TSS code        |
| 8  | **Clean context structures**        | Remove duplicate structs and magic offsets                                     | `task.h`, `context.h`, `task.c`, `idt.asm`  |
| 9  | **Stop scheduler work temporarily** | Leave task switching as an experiment until memory exists                      | `task.*`, `context_switch.asm`              |
| 10 | **M3 — Physical Memory**            | Build the actual physical-frame allocator                                      | new memory files + boot memory-map handling |
| 11 | **M4 — Virtual Memory**             | Page mapping, unmapping, page faults, address spaces                           | new VM files                                |
| 12 | **Kernel heap**                     | `kmalloc`/`kfree` on top of physical + virtual memory                          | new heap files                              |
| 13 | **Return to scheduler**             | Real task stacks/context switching using the memory system                     | `task.*`, `context_switch.asm`, `idt.asm`   |
