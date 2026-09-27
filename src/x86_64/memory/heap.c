#include "heap.h"

#include "pmm.h"
#include "vmm.h"

#include "../../kernel/serial.h"

#include <stdint.h>

#define HEAP_START       0x40000000ULL
#define HEAP_PAGE_SIZE   0x1000ULL

#define BLOCK_FREE       0
#define BLOCK_USED       1

typedef struct heap_block
{
    uint64_t size;
    uint64_t status;

    struct heap_block *next;
    struct heap_block *prev;
} heap_block_t;

static heap_block_t *heap_head = 0;

static uint64_t heap_next_address = HEAP_START;

static uint64_t align_up(uint64_t value, uint64_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/*
 * Map enough physical pages to cover a virtual range.
 */
static int heap_map_pages(
    uint64_t virtual_address,
    uint64_t size
)
{
    uint64_t *pml4 =
        vmm_get_current_pml4();

    if (!pml4)
    {
        return -1;
    }

    uint64_t start =
        virtual_address & ~(HEAP_PAGE_SIZE - 1);

    uint64_t end =
        align_up(
            virtual_address + size,
            HEAP_PAGE_SIZE
        );

    for (
        uint64_t address = start;
        address < end;
        address += HEAP_PAGE_SIZE
    )
    {
        uint64_t physical_address =
            pmm_alloc_page();

        if (physical_address == 0)
        {
            return -1;
        }

        if (vmm_map(pml4,address,physical_address,VMM_WRITABLE) != 0){
            pmm_free_page(physical_address);
            return -1;
        }
    }

    return 0;
}
/*
 * M5 currently uses a simple page-backed heap.
 *
 * The heap grows upward and maintains a linked list
 * of allocated/free blocks.
 */
void kmalloc_init(void)
{
    heap_head = 0;
    heap_next_address = HEAP_START;

    serial_write_str("Kernel heap initialized.\n");
}

/*
 * Find the first free block large enough for the request.
 */
static heap_block_t *find_free_block(uint64_t size)
{
    heap_block_t *current = heap_head;

    while (current)
    {
        if (
            current->status == BLOCK_FREE &&
            current->size >= size
        )
        {
            return current;
        }

        current = current->next;
    }

    return 0;
}

/*
 * Create a new block at the end of the heap.
 */
static heap_block_t *create_block(uint64_t size)
{
    uint64_t total_size =
        sizeof(heap_block_t) + size;

    uint64_t block_start =
        heap_next_address;

    uint64_t block_end =
        block_start + total_size;

    uint64_t mapped_end =
        align_up(
            block_end,
            HEAP_PAGE_SIZE
        );

    uint64_t pages_size =
        mapped_end - block_start;

    /*
     * Allocate physical pages and map them into
     * the kernel heap's virtual address range.
     */
    if (heap_map_pages(
        block_start,
        pages_size
    ) != 0)
    {
        return 0;
    }

    heap_block_t *block =
        (heap_block_t *)block_start;

    block->size = size;
    block->status = BLOCK_USED;
    block->next = 0;
    block->prev = 0;

    if (heap_head == 0)
    {
        heap_head = block;
    }
    else
    {
        heap_block_t *current = heap_head;

        while (current->next)
        {
            current = current->next;
        }

        current->next = block;
        block->prev = current;
    }

    heap_next_address = mapped_end;

    return block;
}

void *kmalloc(uint64_t size)
{
    if (size == 0)
    {
        return 0;
    }

    /*
     * Keep allocations aligned to 16 bytes.
     */
    size = align_up(size, 16);

    heap_block_t *block =
        find_free_block(size);

    if (block)
    {
        block->status = BLOCK_USED;
        return (void *)(block + 1);
    }

    block = create_block(size);

    if (!block)
    {
        return 0;
    }

    return (void *)(block + 1);
}

void kfree(void *ptr)
{
    if (ptr == 0)
    {
        return;
    }

    heap_block_t *block =
        ((heap_block_t *)ptr) - 1;

    block->status = BLOCK_FREE;
}