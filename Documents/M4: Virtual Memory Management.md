# Gorgon OS — M4: Virtual Memory Management

**Status: Basic VMM mapping/unmapping complete.**

M4 has progressed beyond the bootstrap paging environment. Gorgon now has a basic 4 KiB virtual memory manager capable of creating missing page-table levels, mapping virtual addresses to physical pages, reading and writing through those mappings, retrieving PTEs, unmapping pages, and invalidating stale TLB entries.

This document records only functionality that has been implemented and directly verified. Page-fault handling, demand paging, higher-half mappings, user address spaces, and other advanced virtual-memory features remain future work.

---

## 1. Objective

Build the first functional virtual memory manager on top of Gorgon's physical memory manager and existing x86-64 bootstrap page tables.

The immediate goal is to move from a static bootstrap address space toward dynamic 4 KiB virtual-page management.

**Architecture:** x86-64
**Memory model:** Four-level paging
**Page size:** 4 KiB for VMM-managed mappings
**Language:** C / x86-64 Assembly
**Physical allocator:** PMM
**Debug interface:** COM1 Serial
**Virtual hardware:** QEMU

---

## 2. Relationship to M3

M3 established the physical-memory and bootstrap-paging foundation.

The bootstrap environment provides:

```text
PML4
 ↓
PDPT
 ↓
Page Directory
 ↓
2 MiB identity-mapped pages
```

The PMM established dynamic physical-page allocation.

M4 adds:

```text
PML4
 ↓
PDPT
 ↓
Page Directory
 ↓
Page Table
 ↓
4 KiB Page
```

The important distinction is that the bootstrap mapping still exists and continues to provide the initial 1 GiB identity-mapped environment, while the VMM can now create 4 KiB mappings through an L1 Page Table.

---

## 3. VMM Architecture

The VMM follows the x86-64 four-level page-table hierarchy:

```text
Virtual Address
      ↓
PML4
      ↓
PDPT
      ↓
Page Directory
      ↓
Page Table
      ↓
PTE
      ↓
Physical 4 KiB Page
```

A virtual address is divided into four 9-bit indexes plus a 12-bit page offset:

```text
63                         48 47       39 38       30
+----------------------------+-----------+-----------+
|        Sign Extension     | PML4      |   PDPT    |
+----------------------------+-----------+-----------+

29       21 20       12 11                         0
+----------+-----------+-----------------------------+
|    PD    |    PT     |        Page Offset         |
+----------+-----------+-----------------------------+
```

The current implementation uses:

```c
#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)
```

Each table contains 512 64-bit entries and occupies one 4 KiB page.

---

## 4. VMM Interface

The VMM interface is defined in:

```text
src/x86_64/memory/vmm.h
```

Current interface:

```c
int vmm_map(
    uint64_t *pml4,
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t flags
);

int vmm_unmap(
    uint64_t *pml4,
    uint64_t virtual_address
);

uint64_t vmm_get_pte(
    uint64_t *pml4,
    uint64_t virtual_address
);
```

The interface deliberately accepts addresses as values rather than pointers to address variables.

---

## 5. VMM Flags

The current VMM header defines the basic page-entry flags:

```c
#define VMM_PRESENT   (1ULL << 0)
#define VMM_WRITABLE  (1ULL << 1)
#define VMM_USER      (1ULL << 2)
#define VMM_NX        (1ULL << 63)
```

These provide the foundation for later memory protection.

At the current milestone, the verified mapping test uses:

```text
PRESENT
WRITABLE
```

User-mode mappings and NX enforcement have not yet been exercised.

---

## 6. Page-Table Allocation

When `vmm_map()` encounters a missing intermediate page-table entry, it requests a physical page from the PMM.

The hierarchy is created dynamically:

```text
PML4
 └── allocate PDPT if required
      └── allocate PD if required
           └── allocate PT if required
                └── install PTE
```

Newly allocated page tables are explicitly zeroed because:

```text
pmm_alloc_page()
```

only allocates the physical page and does not initialize its contents.

This prevents uninitialized entries from accidentally appearing as present mappings.

---

## 7. Physical-to-Virtual Assumption

The current implementation directly casts physical addresses into pointers when accessing newly allocated page tables.

For example:

```c
uint64_t *new_table = (uint64_t *)new_table_phys;
```

This works under the current bootstrap environment because the first 1 GiB is identity-mapped.

Therefore:

```text
Physical address == Virtual address
```

for the currently relevant memory range.

This is an intentional current-stage assumption.

It will need to change when Gorgon introduces a higher-half kernel or another physical-memory mapping scheme.

---

## 8. Mapping

`vmm_map()` creates a 4 KiB mapping:

```text
Virtual Address
      ↓
PML4
      ↓
PDPT
      ↓
PD
      ↓
PT
      ↓
PTE → Physical Address
```

The physical address is stored in the PTE together with the requested flags and `VMM_PRESENT`.

The mapping operation also executes:

```asm
invlpg
```

for the affected virtual address.

This ensures that a stale TLB translation does not remain active after the page-table entry changes.

---

## 9. Unmapping

`vmm_unmap()` walks the existing hierarchy and clears the target PTE.

It does not automatically free the underlying physical page.

This is intentional because a physical mapping does not necessarily represent PMM-owned memory.

For example, future mappings may refer to:

* PMM-allocated RAM
* MMIO regions
* Framebuffers
* Device memory
* Other reserved physical regions

Therefore:

```text
vmm_unmap()
    ↓
remove virtual mapping

caller
    ↓
decides whether physical memory should be freed
```

The affected TLB entry is invalidated using `invlpg`.

---

## 10. PTE Lookup

`vmm_get_pte()` walks the page-table hierarchy and returns the final PTE.

If any required level does not exist, it returns:

```text
0
```

This provides a simple mechanism for verifying whether a virtual page currently has a mapping.

---

## 11. Bootstrap PML4

The existing bootstrap page tables are defined in:

```text
src/x86_64/boot/main.asm
```

The PML4 is:

```asm
global page_table_l4

page_table_l4:
    resb 4096
```

It is loaded into `CR3` during boot:

```asm
mov eax, page_table_l4
mov cr3, eax
```

The symbol is exported so the C kernel can pass the active PML4 to the VMM.

---

## 12. VMM Verification

The first VMM test was performed after the PMM test.

The test:

1. Allocated a physical page through the PMM.
2. Created a 4 KiB virtual mapping.
3. Wrote a known 64-bit value through the virtual address.
4. Read the value back.
5. Retrieved the PTE.
6. Unmapped the virtual page.
7. Confirmed that the PTE was cleared.
8. Freed the physical page.

The test used a virtual address inside the existing bootstrap identity-mapped range so that the test did not simultaneously introduce a higher-half address-space change.

---

## 13. Verification Result

The kernel produced:

```text
Testing VMM...
VMM map: PASS
VMM read/write: PASS
VMM PTE: PASS
VMM unmap: PASS
VMM PTE cleared: PASS
```

These results directly verify:

| Component                | Status        | Notes                                          |
| ------------------------ | ------------- | ---------------------------------------------- |
| PML4 traversal           | ✅ Verified    | VMM successfully walks the active PML4         |
| PDPT creation            | ✅ Verified    | Missing table can be allocated                 |
| PD creation              | ✅ Verified    | Missing table can be allocated                 |
| PT creation              | ✅ Verified    | Missing L1 table can be allocated              |
| 4 KiB mapping            | ✅ Verified    | Virtual → physical mapping works               |
| Physical page allocation | ✅ Verified    | PMM supplies page to VMM                       |
| Virtual read/write       | ✅ Verified    | Memory accessed through mapped virtual address |
| PTE lookup               | ✅ Verified    | Present PTE successfully retrieved             |
| 4 KiB unmapping          | ✅ Verified    | Mapping successfully removed                   |
| PTE clearing             | ✅ Verified    | Lookup returns no mapping afterward            |
| TLB invalidation         | ✅ Implemented | `invlpg` executed after map/unmap              |
| Physical page freeing    | ✅ Verified    | Test page returned to PMM                      |

---

## 14. PMM Regression Test

The PMM continued to work after integrating the VMM.

Current test output included:

```text
Testing PMM...
Allocated page: 0000000000252000
Free pages after allocation: 0000000000007D8B
PMM alloc count: PASS
Freed page: 0000000000252000
Free pages after freeing: 0000000000007D8C
PMM free count: PASS
PMM test complete.
```

This confirms that introducing the VMM did not break the basic PMM allocation/free accounting.

---

## 15. Current Scope

The current M4 implementation supports:

```text
Physical Memory Manager
        ↓
4 KiB physical page
        ↓
VMM
        ↓
PML4 → PDPT → PD → PT → PTE
        ↓
4 KiB virtual mapping
```

The VMM is currently a basic mapping layer.

It is not yet a complete virtual-memory subsystem.

---

## 16. Not Yet Implemented

The following are intentionally not marked complete:

### Page-fault handling

Not yet implemented as part of the VMM verification.

Future work will inspect:

```text
CR2
```

and the page-fault error code to determine why an access failed.

### Demand paging

Not implemented.

### Higher-half kernel

The kernel remains identity-mapped.

### Per-process address spaces

Not implemented.

### User-mode memory

Not implemented.

### Kernel/user isolation

Not implemented.

### Page-table cleanup

Empty intermediate page tables are currently not reclaimed after unmapping.

### Advanced permission enforcement

The flags exist, but USER and NX behavior have not yet been fully tested.

### Large-page interaction

The bootstrap uses 2 MiB mappings while the new VMM creates 4 KiB mappings. A complete design for safely managing interactions between these mapping types remains future work.

---

## 17. Known Technical Limitations

### Identity-mapped table access

The VMM currently assumes physical page-table addresses are directly dereferenceable.

This must be redesigned when Gorgon moves to a higher-half or other non-identity physical-memory layout.

### Intermediate permission propagation

New intermediate entries inherit the requested USER flag.

However, an existing intermediate entry is not retroactively upgraded if a later mapping requests USER access.

This is acceptable for the current kernel-only stage but must be addressed before real user address spaces are introduced.

### Physical page ownership

`vmm_unmap()` does not know whether the physical page belongs to the PMM.

Ownership remains the caller's responsibility.

### Page-table reclamation

When the final mapping underneath a page table disappears, the empty page-table pages are not currently returned to the PMM.

---

## 18. Next Work

The next VMM work should focus on fault behavior rather than immediately moving to higher-half paging.

Planned progression:

```text
Basic 4 KiB mapping
        ↓
Page-fault handler
        ↓
CR2 / error-code reporting
        ↓
Intentional unmapped-access test
        ↓
Multiple-page mapping tests
        ↓
VMM robustness / cleanup
        ↓
Higher-half address-space design
```

The kernel heap remains a separate milestone after the virtual-memory foundation is sufficiently reliable.

---

## M4 Current Result

**M4 — Basic Virtual Memory Management: COMPLETE for the current mapping/unmapping scope.**

Gorgon now has a working VMM capable of:

* Walking the x86-64 four-level hierarchy.
* Allocating missing page-table levels through the PMM.
* Zeroing newly allocated page tables.
* Creating 4 KiB virtual-to-physical mappings.
* Applying basic page flags.
* Accessing memory through a newly created virtual mapping.
* Reading the resulting PTE.
* Removing mappings.
* Invalidating affected TLB entries.
* Returning physical pages to the PMM when explicitly requested by the caller.

The remaining page-fault, protection, address-space, and higher-half work is deliberately carried forward rather than being represented as completed.

**M4 basic VMM mapping is done.**

