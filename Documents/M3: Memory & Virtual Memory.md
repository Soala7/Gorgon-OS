# Gorgon OS — M3: Memory & Virtual Memory

## Objective

Establish the initial physical and virtual memory foundation required for Gorgon to manage memory beyond the bootstrap paging environment.

**Architecture:** x86-64

**Memory model:** Paging / Virtual Memory

**Page size:** 2 MiB bootstrap pages

**Language:** C / x86-64 Assembly

**Debug interface:** COM1 Serial

**Virtual hardware:** QEMU

---

# 1. Memory Architecture

Gorgon uses the x86-64 paging architecture to translate virtual addresses into physical addresses.

The current address-translation hierarchy is:

```text
Virtual Address
      ↓
PML4 / L4
      ↓
PDPT / L3
      ↓
Page Directory / L2
      ↓
2 MiB Page
      ↓
Physical Memory
```

This provides the foundation for Gorgon's future virtual memory manager and process address spaces.

---

# 2. Bootstrap Page Tables

During early boot, Gorgon creates the page tables required to enable paging and enter long mode.

The current bootstrap implementation contains three page-table pages:

```text
PML4 / L4
PDPT / L3
Page Directory / L2
```

Each table occupies one 4 KiB page.

```text
L4 → 4096 bytes
L3 → 4096 bytes
L2 → 4096 bytes
```

The kernel stack is allocated separately and is not part of the page-table hierarchy.

---

# 3. Identity Mapping

The initial memory space uses identity mapping.

This means:

```text
Virtual Address = Physical Address
```

For example:

```text
Virtual 0x00200000
        ↓
Physical 0x00200000
```

This allows the kernel to continue executing using the same addresses after paging is enabled.

---

# 4. 2 MiB Pages

Gorgon currently uses 2 MiB pages for its bootstrap address space.

The Page Directory contains 512 entries.

Each entry maps:

```text
2 MiB
```

Therefore the current bootstrap mapping covers:

```text
512 × 2 MiB = 1 GiB
```

of identity-mapped memory.

The page entries are configured as:

```text
Present
Writable
Huge Page
```

The huge-page flag allows the Page Directory entries to directly map 2 MiB pages without requiring another page-table level.

---

# 5. Loading the Page Tables

The physical address of the PML4 is loaded into the CPU's `CR3` register:

```asm
mov eax, page_table_l4
mov cr3, eax
```

This tells the processor where the active top-level page table begins.

---

# 6. Enabling PAE

Before entering long mode, Gorgon enables Physical Address Extension through `CR4`.

The PAE bit is:

```text
CR4 bit 5
```

The boot code enables it with:

```asm
mov eax, cr4
or eax, 1 << 5
mov cr4, eax
```

PAE is required for the x86-64 paging architecture used by long mode.

---

# 7. Enabling Long Mode

Gorgon enables long mode through the Extended Feature Enable Register (`EFER`).

The register is accessed using:

```asm
rdmsr
wrmsr
```

The Long Mode Enable bit is:

```text
EFER.LME = bit 8
```

Gorgon sets this bit before enabling paging.

---

# 8. Enabling Paging

Paging is enabled through `CR0`.

The Paging Enable bit is:

```text
CR0 bit 31
```

Gorgon enables it using:

```asm
mov eax, cr0
or eax, 1 << 31
mov cr0, eax
```

Once paging is enabled together with the required long-mode configuration, the processor can execute in the x86-64 virtual-memory environment.

---

# 9. Kernel Stack

Gorgon also provides a dedicated bootstrap stack.

The current stack allocation is:

```text
4 × 4096 bytes
```

giving:

```text
16 KiB
```

of initial kernel stack space.

The stack is aligned before kernel execution begins.

The stack is separate from the page-table structures.

---

# 10. Current Memory Layout

The current bootstrap memory arrangement is conceptually:

```text
Gorgon Bootstrap Memory

Page Tables
├── PML4 / L4       4 KiB
├── PDPT / L3       4 KiB
└── Page Directory  4 KiB

Kernel Stack
└── Bootstrap Stack 16 KiB

Identity Mapping
└── 0 → 1 GiB
    using 2 MiB pages
```

This is intentionally simple and exists to provide the minimum memory environment required for the kernel to operate.

---

# 11. Memory Protection Foundation

The current page entries use the basic:

```text
Present
Writable
```

permissions.

This provides the foundation for future memory protection.

Later memory-management work will introduce:

* Read/write protection
* User/kernel address separation
* Guard pages
* Per-process address spaces
* Page allocation and deallocation
* Memory isolation
* Kernel heap management

---

# 12. Current Scope

The current implementation is a **bootstrap virtual-memory system**, not yet a complete memory manager.

The following systems remain future work:

```text
Physical Memory Manager
        ↓
Page Frame Allocator
        ↓
Virtual Memory Manager
        ↓
Kernel Heap
        ↓
Process Address Spaces
        ↓
User/Kernel Memory Protection
```

These components will be developed as Gorgon progresses toward full process isolation and userspace execution.

---

# 13. M3 Verification Checklist

| Component                     | Status |
| ----------------------------- | ------ |
| x86-64 paging architecture    | ✓      |
| PML4 / L4                     | ✓      |
| PDPT / L3                     | ✓      |
| Page Directory / L2           | ✓      |
| 4 KiB page-table allocation   | ✓      |
| 2 MiB pages                   | ✓      |
| Identity mapping              | ✓      |
| 512 × 2 MiB entries           | ✓      |
| 1 GiB bootstrap mapping       | ✓      |
| `CR3` configuration           | ✓      |
| PAE enabled                   | ✓      |
| EFER.LME enabled              | ✓      |
| Paging enabled                | ✓      |
| Bootstrap kernel stack        | ✓      |
| Memory execution under paging | ✓      |

---

# M3 Result

**M3 — Memory & Virtual Memory: COMPLETE**

Gorgon now has a working x86-64 bootstrap memory environment capable of:

* Creating and loading a multi-level page-table hierarchy.
* Using 2 MiB pages.
* Identity-mapping the initial 1 GiB address space.
* Enabling PAE.
* Enabling long mode.
* Enabling CPU paging.
* Providing a dedicated bootstrap kernel stack.
* Executing the kernel with virtual address translation active.

This establishes the memory foundation required for Gorgon's future physical memory manager, virtual memory manager, kernel heap, and isolated process address spaces.

**M3 is done.**
