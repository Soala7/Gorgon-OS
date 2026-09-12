# M1 Completion: Build, Boot, and Debug Foundation

## Objective

The goal of M1 was to establish a reliable development loop for the kernel:

- Build a freestanding x86-64 kernel
- Boot it with GRUB
- Verify Multiboot2 entry
- Reach a working entry point in C
- Emit serial output over COM1
- Attach QEMU and GDB for live debugging

## Result

M1 is complete as a foundational milestone. The project now has the basic boot and debug flow required to continue into M2.

## Completed components

### 1. Build pipeline

The project produces a kernel binary and bootable ISO from the source tree using a cross-compiler and linker pipeline.

Representative flow:

```text
source files
  -> assembler / compiler
  -> object files
  -> linker
  -> kernel.bin
  -> GRUB ISO
  -> QEMU boot
```

Artifacts include:

- `dist/x86_64/kernel.bin`
- `dist/x86_64/gorgon.iso`

### 2. GRUB + Multiboot2 support

The kernel is launched from GRUB using a Multiboot2-compatible entry flow. The boot code verifies the Multiboot2 magic value before proceeding.

### 3. Early x86-64 startup

The startup path includes:

- Multiboot2 validation
- CPUID verification
- long mode detection
- initial page table setup
- PAE and long mode enablement
- GDT load
- far jump into 64-bit code
- IDT initialization
- transition into `kernel_main()`

### 4. GDT

The project includes a basic x86-64 GDT with:

- null descriptor
- 64-bit code segment
- data segment

This is required for proper long-mode execution.

### 5. Early paging

A minimal bootstrap paging setup is present. It establishes a small identity-mapped region and prepares the machine for 64-bit execution.

### 6. Serial debugging

The project exposes a minimal serial driver for debugging via COM1. The implementation uses the standard UART I/O ports and writes data directly to the serial transmit register.

### 7. Debugging workflow

The project supports the required debugging loop:

- build kernel
- launch QEMU
- read serial output
- attach GDB
- inspect memory and control flow

## Current known state

The project is at the early kernel stage. It is not yet a complete kernel, but the foundation is in place for the next milestone.

## Remaining M1-quality items

The next phase should continue to tighten the following items:

- cleaner startup validation and error handling
- stable serial output testing
- complete long-mode entry checks
- consistent logging and panic output
- reproducible debug script and GDB setup

## Summary

M1 establishes the required boot pathway and the minimum debug toolchain needed to proceed into the architecture and memory milestones.
