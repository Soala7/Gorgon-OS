# CPU Startup Sequence Reference

## Target

x86-64 bare-metal startup path for Gorgon.

## Sequence

```text
GRUB loads kernel
  -> Multiboot2 header recognized
  -> control enters kernel _start
  -> verify Multiboot2 magic value
  -> verify CPUID support
  -> verify long mode support
  -> build initial page tables
  -> enable PAE
  -> enable long mode
  -> load GDT
  -> far jump to 64-bit code
  -> initialize IDT
  -> call kernel_main()
```

## Notes

This is the minimum startup path needed before the kernel can begin memory management, process work, or full device initialization.
