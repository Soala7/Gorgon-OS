# Gorgon OS — M3: Memory & Virtual Memory

**Status: Complete for the current M3 scope.** The bootstrap virtual-memory environment is operational and the Physical Memory Manager (PMM) has been implemented and verified. The bootstrap paging environment and PMM were tested during development. The remaining Virtual Memory Manager (VMM), kernel heap, and process address-space work belong to later milestones.

Nothing below is marked complete unless it reflects the current implementation and observed verification.

---

## 1. Objective

Establish the initial physical and virtual memory foundation required for Gorgon to manage memory beyond the bootstrap paging environment.

**Architecture:** x86-64
**Memory model:** Paging / Virtual Memory
**Page size:** 4 KiB physical frames; 2 MiB bootstrap pages
**Language:** C / x86-64 Assembly
**Debug interface:** COM1 Serial
**Virtual hardware:** QEMU

This milestone establishes two foundations:

1. The bootstrap paging environment required to enter and execute in x86-64 long mode.
2. The initial Physical Memory Manager responsible for tracking and allocating physical pages.

A complete Virtual Memory Manager is **not** part of the current implementation.

---

## 2. Memory Architecture

Gorgon uses the x86-64 four-level paging architecture to translate virtual addresses into physical addresses.

The bootstrap translation hierarchy is:

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

The bootstrap mapping terminates at the Page Directory because the L2 entries use the Huge Page flag and therefore map 2 MiB pages directly.

The later VMM will introduce 4 KiB page mappings through the L1 Page Table level where required.

---

## 3. Bootstrap Page Tables

During early boot, Gorgon creates the page tables required to enable paging and enter long mode.

The current bootstrap implementation contains:

```text
PML4 / L4
PDPT / L3
Page Directory / L2
```

Each table occupies one 4 KiB page:

```text
L4 → 4096 bytes
L3 → 4096 bytes
L2 → 4096 bytes
```

The kernel bootstrap stack is allocated separately from the page-table hierarchy.

At the L4 and L3 levels, a single entry is populated to reach the L2 table.

The L2 table contains the actual 2 MiB mappings.

---

## 4. Identity Mapping

The initial memory space uses identity mapping:

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

Identity mapping is a bootstrap mechanism rather than the long-term Gorgon address-space design.

Future memory-management work will introduce more deliberate virtual-address layouts, including separate kernel and userspace regions.

---

## 5. 2 MiB Bootstrap Pages

Gorgon currently uses 2 MiB pages for its bootstrap address space.

The Page Directory contains 512 entries. Each entry maps 2 MiB:

```text
512 × 2 MiB = 1 GiB
```

The bootstrap therefore provides a theoretical 1 GiB identity-mapped range.

The L2 entries are configured as:

```text
Present
Writable
Huge Page
```

The Huge Page flag allows an L2 entry to directly map a 2 MiB page without requiring an L1 page table.

### Important limitation

The 1 GiB figure represents the **configured virtual mapping**, not 1 GiB of confirmed usable physical RAM.

The bootstrap mapping does not use the Multiboot2 memory map to dynamically construct the page tables. Physical-memory availability is instead handled separately by the PMM.

---

## 6. Loading the Page Tables

The physical address of the PML4 is loaded into `CR3`:

```asm
mov eax, page_table_l4
mov cr3, eax
```

`CR3` contains the physical address of the active top-level page table.

This is required because the MMU uses `CR3` to locate the page-table hierarchy.

---

## 7. Enabling PAE

Physical Address Extension is enabled through `CR4`.

The PAE bit is bit 5:

```asm
mov eax, cr4
or eax, 1 << 5
mov cr4, eax
```

PAE is required for the x86-64 paging architecture used by long mode.

---

## 8. Enabling Long Mode

Gorgon enables long mode through the Extended Feature Enable Register (`EFER`).

The Long Mode Enable bit is:

```text
EFER.LME = bit 8
```

The boot code accesses the register using:

```asm
rdmsr
wrmsr
```

LME is enabled before paging is turned on.

The CPU enters long mode after the required paging configuration is active.

---

## 9. Enabling Paging

Paging is enabled through `CR0`.

The Paging Enable bit is bit 31:

```asm
mov eax, cr0
or eax, 1 << 31
mov cr0, eax
```

The bootstrap sequence is:

```text
PAE (CR4)
   ↓
LME (EFER)
   ↓
Paging (CR0)
   ↓
x86-64 long-mode execution
```

---

## 10. Kernel Stack

Gorgon provides a dedicated bootstrap stack:

```text
4 × 4096 bytes = 16 KiB
```

The boot code initializes the stack before entering the kernel.

The stack is separate from the page-table structures.

The current 16 KiB stack is sufficient for the bootstrap path, but its size is provisional and will need to be reconsidered as interrupt handling, processes, threads, and deeper kernel call chains are introduced.

---

# 11. Physical Memory Manager

The Physical Memory Manager is now implemented.

The PMM uses the Multiboot2 memory map to identify available physical memory and maintains a bitmap representing the state of physical pages.

The current PMM uses:

```text
4 KiB physical pages
Bitmap-based allocation
Multiboot2 memory map
Free-page accounting
Page allocation
Page freeing
```

### Bitmap capacity

The current bitmap is statically sized at:

```text
1 MiB
```

At one bit per 4 KiB page:

```text
1 MiB × 8 bits
= 8,388,608 pages

8,388,608 × 4 KiB
= 32 GiB
```

Therefore the current bitmap can represent up to approximately **32 GiB of physical memory**.

This is a deliberate development limitation and is sufficient for the current QEMU environment.

---

## 12. PMM Initialization

During initialization, the PMM:

1. Reads the Multiboot2 memory map.
2. Determines the highest available physical address.
3. Calculates the number of physical pages.
4. Initializes the frame bitmap.
5. Marks pages belonging to available memory regions as free.
6. Reserves the kernel image.
7. Reserves the Multiboot information structure.
8. Reserves physical page zero.
9. Maintains a free-page counter.

The PMM therefore does not treat every physical address as automatically available.

Reserved memory is excluded from allocation.

---

## 13. Physical Page Allocation

`pmm_alloc_page()` searches the bitmap for a free physical page.

When a free page is found:

```text
Free
 ↓
Allocated
```

The corresponding bitmap bit is set and the free-page counter is decremented.

The allocator returns the physical address of the page.

Example:

```text
Page number:
0x0001

Physical address:
0x00001000
```

Physical page zero is reserved because address `0` is used as the allocation-failure return value.

---

## 14. Physical Page Freeing

`pmm_free_page()` accepts a page-aligned physical address.

A page can be returned to the free pool when it is no longer allocated.

The PMM validates:

* Page alignment
* Non-zero address
* Page range
* Current allocation state

When a valid allocated page is freed:

```text
Allocated
    ↓
Free
```

The bitmap is cleared and the free-page counter is incremented.

---

## 15. PMM Verification

The PMM was tested directly through the kernel's serial output.

The observed test was:

```text
PMM initialized.
Total pages: 0000000000007FE0
Free pages: 0000000000007E2F
Testing PMM...
PMM alloc: searching...
PMM alloc: found page 0000000000000001
Allocated page: 0000000000001000
Free pages after allocation: 0000000000007E2E
PMM alloc count: PASS
Freed page: 0000000000001000
Free pages after freeing: 0000000000007E2F
PMM free count: PASS
PMM test complete.
```

The test confirms:

```text
PMM initialization       PASS
Page allocation          PASS
Free-page decrement      PASS
Page freeing             PASS
Free-page restoration    PASS
```

The free-page count decreased by exactly one page after allocation and returned to its original value after the page was freed.

---

## 16. Current Memory Layout

The current bootstrap memory arrangement is conceptually:

```text
Gorgon Bootstrap Memory

Page Tables
├── PML4 / L4       4 KiB
├── PDPT / L3       4 KiB
└── Page Directory  4 KiB

Kernel Stack
└── Bootstrap Stack 16 KiB

Bootstrap Mapping
└── 0 → 1 GiB
    using 2 MiB pages

Physical Memory Manager
└── 4 KiB physical-page bitmap
```

The page tables provide the initial virtual-memory environment.

The PMM manages physical-page availability independently of the bootstrap mapping.

---

## 17. Memory Protection Foundation

The bootstrap page entries currently use:

```text
Present
Writable
```

There is currently no:

* User/kernel page separation
* Read-only kernel text enforcement
* NX enforcement
* Guard-page system
* Per-process address space
* Full virtual-memory allocation API

The current configuration is therefore a kernel-only bootstrap environment.

Future memory-management work will introduce:

* Read/write protection
* User/kernel address separation
* Guard pages
* Per-process address spaces
* Page allocation and deallocation
* Memory isolation
* Kernel heap management
* NX support where appropriate

---

## 18. Current Scope

The completed M3 foundation can be represented as:

```text
Multiboot2 Memory Map
        ↓
Physical Memory Manager
        ↓
Physical Page Allocation
        ↓
Bootstrap Paging
        ↓
x86-64 Virtual Address Translation
```

The next memory-management layer is:

```text
Physical Memory Manager
        ↓
Virtual Memory Manager
        ↓
Kernel Heap
        ↓
Process Address Spaces
        ↓
User/Kernel Memory Protection
```

The VMM is therefore the next major memory subsystem rather than something already completed by M3.

---

## 19. Verification Checklist

| Component                        | Status     | Notes                                                      |
| -------------------------------- | ---------- | ---------------------------------------------------------- |
| x86-64 paging architecture       | ✅ Done     | Four-level hierarchy established                           |
| PML4 / L4                        | ✅ Done     | Single entry populated                                     |
| PDPT / L3                        | ✅ Done     | Single entry populated                                     |
| Page Directory / L2              | ✅ Done     | 512 entries; terminal level for 2 MiB bootstrap pages      |
| 4 KiB page-table allocation      | ✅ Done     | L4/L3/L2 each occupy one 4 KiB page                        |
| 2 MiB bootstrap pages            | ✅ Done     | Huge Page flag set on L2 entries                           |
| Identity mapping                 | ✅ Done     | Bootstrap virtual addresses equal physical addresses       |
| 512 × 2 MiB entries              | ✅ Done     | Full L2 bootstrap mapping populated                        |
| 1 GiB bootstrap mapping          | ✅ Done     | Configured mapping; not equivalent to confirmed usable RAM |
| `CR3` configuration              | ✅ Done     | PML4 physical address loaded                               |
| PAE enabled                      | ✅ Done     | CR4 bit 5 set                                              |
| EFER.LME enabled                 | ✅ Done     | EFER bit 8 set before paging                               |
| Paging enabled                   | ✅ Done     | CR0 bit 31 set                                             |
| Bootstrap kernel stack           | ✅ Done     | 16 KiB, separate from page tables                          |
| Kernel executes under paging     | ✅ Done     | Kernel reaches normal execution                            |
| Multiboot2 memory-map processing | ✅ Done     | PMM reads available regions                                |
| Physical page bitmap             | ✅ Done     | Bitmap-based PMM implemented                               |
| Physical page allocation         | ✅ Done     | Allocation verified                                        |
| Physical page freeing            | ✅ Done     | Freeing verified                                           |
| Free-page accounting             | ✅ Done     | Allocation/deallocation counts verified                    |
| Kernel/Multiboot reservation     | ✅ Done     | PMM reserves these ranges                                  |
| Page zero reservation            | ✅ Done     | Prevents `0` from being returned as a valid page           |
| Virtual Memory Manager           | ⬜ Not done | Next memory-management milestone                           |
| 4 KiB virtual mappings           | ⬜ Not done | Requires VMM/page-table management                         |
| Higher-half mapping              | ⬜ Not done | Future address-space design                                |
| NX / read-only enforcement       | ⬜ Not done | Future protection work                                     |
| TLB invalidation on remap        | ⬜ Not done | Required once live page tables are modified                |
| Kernel heap                      | ⬜ Not done | Future milestone                                           |
| Per-process address spaces       | ⬜ Not done | Future process/memory work                                 |

---

## 20. Known Gaps Carried Forward

These limitations are intentionally recorded rather than treated as completed functionality.

### Bootstrap paging limitations

* The bootstrap mapping is identity-mapped.
* The initial mapping covers a theoretical 1 GiB.
* The bootstrap mapping is not dynamically constructed from the Multiboot2 memory map.
* There is no higher-half kernel mapping yet.
* There is no user/kernel virtual-address separation.
* Page permissions remain basic.
* TLB management is not yet required because the current bootstrap mapping is static.

### PMM limitations

* The bitmap is statically limited to approximately 32 GiB of physical memory.
* The current PMM tests basic allocation and freeing rather than exhaustive allocator stress.
* Page-table and boot-time memory reservations will need continued auditing as more kernel subsystems are added.
* The PMM is a physical frame allocator; it does not provide virtual-address mapping.

### Future VMM requirements

The future VMM will need to provide:

* Creation and modification of page tables
* 4 KiB virtual-page mappings
* Mapping and unmapping
* Permission flags
* TLB invalidation
* Kernel virtual-address management
* Per-process address spaces
* User/kernel isolation

---

# M3 Result

**M3 — Memory & Virtual Memory: COMPLETE FOR CURRENT SCOPE**

Gorgon now has:

* A working x86-64 bootstrap paging environment.
* PML4, PDPT, and Page Directory structures.
* 2 MiB bootstrap pages.
* A 1 GiB identity-mapped bootstrap range.
* PAE enabled.
* Long mode enabled.
* CPU paging enabled.
* A dedicated 16 KiB bootstrap stack.
* Multiboot2 memory-map processing.
* A bitmap-based Physical Memory Manager.
* 4 KiB physical-page allocation.
* Physical-page freeing.
* Free-page accounting.
* Reservation of critical physical memory ranges.
* Verified PMM allocation and free tests.

The PMM test demonstrated that allocating one page decreases the free-page count by one and freeing that page restores the count.

The remaining work — the actual Virtual Memory Manager, kernel heap, and process address spaces — is intentionally carried forward to later milestones.

**M3 is complete within the scope defined above. The next memory milestone is M4 — Virtual Memory Management.**

