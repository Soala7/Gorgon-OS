# Gorgon OS - M5: Kernel Heap

**Status: Basic kernel heap implemented; initialization, allocation, free, and block reuse are smoke-tested.**

M5 adds a small kernel heap on top of the PMM and VMM. The heap provides byte-sized allocations to kernel code while obtaining its backing memory as physical pages mapped into a dedicated virtual range.

This milestone documents the current first implementation. It is not a general-purpose or production-ready allocator.

---

## 1. Objective

Provide the kernel with basic dynamic memory allocation:

- Initialize a heap after the VMM has selected the current address space.
- Allocate aligned blocks with `kmalloc()`.
- Mark blocks free with `kfree()`.
- Reuse a freed block when it is large enough for a later request.
- Obtain heap backing pages through the PMM and map them through the VMM.

**Architecture:** x86-64  
**Page size:** 4 KiB  
**Heap virtual base:** `0x40000000`  
**Backing allocator:** PMM  
**Mapping layer:** VMM  
**Debug interface:** COM1 serial  
**Virtual hardware:** QEMU

---

## 2. Interface

The API is declared in:

```text
src/x86_64/memory/heap.h
```

```c
void kmalloc_init(void);
void *kmalloc(uint64_t size);
void kfree(void *ptr);
```

`kmalloc(0)` returns `NULL`. A failed allocation also returns `NULL`. `kfree(NULL)` is a no-op.

---

## 3. Allocation Design

The heap grows upward from `0x40000000`. Each allocation has a metadata header immediately before its returned payload:

```text
+----------------------+-------------------------+
| block metadata       | allocation payload     |
+----------------------+-------------------------+
```

The metadata tracks the payload size, whether the block is free or in use, and the previous and next blocks in a doubly linked list.

Allocation sizes are rounded up to 16-byte boundaries. The allocator scans the list from the beginning and uses the first free block whose payload is large enough. If there is no suitable free block, it creates a new block at the heap end.

When extending the heap, the allocator rounds the required virtual range to 4 KiB pages, allocates physical pages from the PMM, and maps them writable through the current address space with the VMM. The VMM currently relies on the bootstrap identity mapping to access physical page-table memory.

---

## 4. Initialization and Ordering

The current kernel startup order is:

```text
pmm_init()
    ↓
vmm_init()
    ↓
kmalloc_init()
    ↓
heap smoke test
    ↓
timer_init()
```

The heap must be initialized after the VMM has established the current PML4 because page allocation uses that address space to map heap pages.

`kmalloc_init()` resets the heap list and virtual growth pointer. It does not release previously mapped pages, so it is intended for initial kernel startup rather than repeated runtime reset.

---

## 5. Current Smoke Test

`kernel_main()` exercises the allocator by:

1. Allocating 16, 64, and 4096 bytes.
2. Checking that all three allocations are non-null.
3. Freeing the 64-byte allocation.
4. Requesting 32 bytes and checking that the freed block is reused.
5. Freeing the remaining allocations.
6. Continuing to timer initialization and the timer tick loop.

The expected heap-related serial output is:

```text
Kernel heap initialized.
Testing kernel heap...
kmalloc: PASS
kfree/reuse: PASS
Kernel heap test complete.
```

The post-heap timer output also verifies that heap initialization did not prevent the kernel from continuing through interrupt setup.

---

## 6. Current Limitations

The allocator is intentionally minimal:

- Free blocks are not split when a smaller allocation reuses them.
- Adjacent free blocks are not coalesced.
- `kfree()` does not validate that its argument belongs to the heap or detect double frees.
- Freed blocks remain mapped, and the heap does not return backing pages to the PMM.
- A failure partway through mapping a larger heap range does not yet roll back pages already allocated and mapped in that attempt.
- The linked-list operations are not synchronized for concurrent kernel execution.
- The heap range and physical-page access assumptions are specific to the current identity-mapped bootstrap environment.

These are future allocator and memory-management improvements, not guarantees of this milestone.

---

## 7. Next Steps

Potential follow-up work includes block splitting and coalescing, allocator input validation, locking, rollback on mapping failure, heap growth limits, and tests for allocation failure and larger workloads. Heap pages should only be reclaimed once the allocator can prove that an entire mapped page is unused.
