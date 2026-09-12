| Instruction | Full meaning           | What it does                                        |
| ----------- | ---------------------- | --------------------------------------------------- |
| `mov`       | Move                   | Copies data from one place to another               |
| `lea`       | Load Effective Address | Calculates an address                               |
| `cmp`       | Compare                | Compares two values by setting CPU flags            |
| `test`      | Test                   | Performs AND without storing the result; sets flags |
| `jmp`       | Jump                   | Unconditionally jumps to a label/address            |
| `je`        | Jump if Equal          | Jumps when the Zero Flag is set                     |
| `jne`       | Jump if Not Equal      | Jumps when Zero Flag is clear                       |
| `jb`        | Jump Below             | Unsigned `<` comparison                             |
| `call`      | Call procedure         | Jumps to a function and saves return address        |
| `ret`       | Return                 | Returns from a `call`                               |
| `push`      | Push                   | Places a value onto the stack                       |
| `pop`       | Pop                    | Removes a value from the stack                      |
| `sub`       | Subtract               | Subtracts a value                                   |
| `add`       | Add                    | Adds a value                                        |
| `inc`       | Increment              | Adds 1                                              |
| `dec`       | Decrement              | Subtracts 1                                         |
| `xor`       | Exclusive OR           | Bitwise XOR; often used to zero a register          |
| `or`        | OR                     | Bitwise OR                                          |
| `and`       | AND                    | Bitwise AND                                         |
| `shr`       | Shift Right            | Moves bits to the right                             |
| `shl`       | Shift Left             | Moves bits to the left                              |
| `imul`      | Integer Multiply       | Multiplies integers                                 |
| `hlt`       | Halt                   | Stops CPU execution until an interrupt/reset        |
| `cli`       | Clear Interrupt Flag   | Disables maskable interrupts                        |
| `sti`       | Set Interrupt Flag     | Enables maskable interrupts                         |
| `lgdt`      | Load GDT Register      | Loads the Global Descriptor Table                   |
| `lidt`      | Load IDT Register      | Loads the Interrupt Descriptor Table                |
| `iretq`     | Interrupt Return       | Returns from a 64-bit interrupt/exception           |

## Assembly notes for the kernel

### 1. `global` and `extern`

`global` exposes a label so other files can reference it at link time. `extern` tells the assembler that a symbol exists elsewhere and will be resolved by the linker.

This is how the bootstrap code calls `gdt_load`, `idt_load`, and other kernel functions without defining them in the same source file.

### 2. `lea` vs `mov`

`lea` calculates an address and stores that address in a register. It does not dereference memory.

Example:

```asm
lea rdi, [rel idt]
```

This loads the address of the IDT table into `rdi` rather than reading the data stored inside it.

### 3. IDT gate construction

Each IDT entry is 16 bytes wide. A vector number is converted to an entry offset with:

```asm
imul rsi, 16
```

Then the handler address is split into low, middle, and high bits and placed into the gate fields.

The gate byte `0x8E` is the usual 64-bit interrupt gate attribute:

- present bit set
- privilege level 0
- interrupt gate type

### 4. Why `cli` is used

`cli` clears the interrupt flag so the CPU does not accept maskable interrupts while the IDT or exception handling path is being initialized.

This avoids partially configured interrupt state during boot and early fault handling.

### 5. `call` and stack alignment

The x86-64 System V ABI expects the stack to be properly aligned before a function call. In the exception path, the code does:

```asm
push rdi
sub rsp, 8
```

This preserves the exception vector and keeps the stack aligned before calling a C function like `serial_write_str`.

### 6. Exception handler pattern

Each exception handler usually does the following:

1. put its vector number in `rdi`
2. jump to a common `exception_handler`
3. print the vector number through serial output
4. halt the CPU or enter a panic path

This keeps the assembly small and keeps the hardware-specific details isolated from the higher-level debug logic.

### 7. Boot sequence summary

The early boot flow is:

```text
GRUB loads kernel
-> verify Multiboot2
-> verify CPUID support
-> verify long mode support
-> build page tables
-> enable paging and long mode
-> load GDT
-> load IDT
-> jump to 64-bit kernel entry
```

### 8. Good assembly style

The goal is to keep assembly readable without burying the file under large explanatory comment blocks. The important rules are:

- use clearly named labels
- keep the logic direct and compact
- put broader explanations in a reference file like this one
- use only minimal inline comments for critical CPU-specific details

This keeps the source code closer to the way real kernel code is typically written: compact, precise, and documented in a reference layer instead of repeated in every function.

