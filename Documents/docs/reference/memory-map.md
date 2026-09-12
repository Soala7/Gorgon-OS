# Memory Map Reference

## Early boot assumptions

At M1, Gorgon is still in an early bootstrap state. The memory model is intentionally simple and is meant to support initial validation rather than production-safe memory management.

## Typical early layout

```text
Low memory / BIOS / boot data
  -> GRUB structures
  -> kernel image
  -> initial page tables
  -> reserved memory
  -> available RAM for allocator later
```

## Notes

This is a placeholder reference file for the early phase. The full physical-memory and virtual-memory layout will be described in later milestones when the allocator and page tables become production-grade.
