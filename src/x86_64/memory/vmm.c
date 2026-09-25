#include "vmm.h"
#include "pmm.h"

/*
 * Current assumption:
 *
 * Physical addresses can be directly dereferenced because the
 * bootstrap page tables identity-map the first 1 GiB of memory.
 *
 * This will need to change when Gorgon moves to a higher-half
 * virtual address layout.
 */

#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr)   (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr)   (((addr) >> 12) & 0x1FF)

#define TABLE_ADDRESS_MASK (~0xFFFULL)

/*
 * Walk one level of the page-table hierarchy.
 *
 * If the requested table does not exist, allocate a physical
 * page through the PMM and use it as the new page table.
 *
 * Newly allocated page tables are zeroed because pmm_alloc_page()
 * does not initialize their contents.
 */
static uint64_t *get_or_create_table(
    uint64_t *table,
    uint64_t index,
    uint64_t flags
)
{
    if (!(table[index] & VMM_PRESENT))
    {
        uint64_t new_table_phys = pmm_alloc_page();

        if (new_table_phys == 0)
            return 0;

        uint64_t *new_table = (uint64_t *)new_table_phys;

        for (int i = 0; i < 512; i++)
            new_table[i] = 0;

        /*
         * Intermediate page-table entries must be present and
         * writable. USER is propagated when requested.
         */
        table[index] =
            new_table_phys |
            VMM_PRESENT |
            VMM_WRITABLE |
            (flags & VMM_USER);
    }

    return (uint64_t *)(table[index] & TABLE_ADDRESS_MASK);
}

/*
 * Map one 4 KiB virtual page to one 4 KiB physical page.
 */
int vmm_map(
    uint64_t *pml4,
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t flags
)
{
    uint64_t *pdpt =
        get_or_create_table(
            pml4,
            PML4_INDEX(virtual_address),
            flags
        );

    if (!pdpt)
        return -1;

    uint64_t *pd =
        get_or_create_table(
            pdpt,
            PDPT_INDEX(virtual_address),
            flags
        );

    if (!pd)
        return -1;

    uint64_t *pt =
        get_or_create_table(
            pd,
            PD_INDEX(virtual_address),
            flags
        );

    if (!pt)
        return -1;

    uint64_t index = PT_INDEX(virtual_address);

    /*
     * Install the final page-table entry.
     */
    pt[index] =
        (physical_address & TABLE_ADDRESS_MASK) |
        (flags | VMM_PRESENT);

    /*
     * Ensure the CPU does not continue using an old cached
     * translation for this virtual address.
     */
    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 0;
}

/*
 * Remove the mapping for one 4 KiB virtual page.
 *
 * The physical page itself is NOT freed here because the mapping
 * may refer to PMM memory, MMIO, framebuffer memory, etc.
 */
int vmm_unmap(
    uint64_t *pml4,
    uint64_t virtual_address
)
{
    if (!(pml4[PML4_INDEX(virtual_address)] & VMM_PRESENT))
        return -1;

    uint64_t *pdpt =
        (uint64_t *)(
            pml4[PML4_INDEX(virtual_address)] &
            TABLE_ADDRESS_MASK
        );

    if (!(pdpt[PDPT_INDEX(virtual_address)] & VMM_PRESENT))
        return -1;

    uint64_t *pd =
        (uint64_t *)(
            pdpt[PDPT_INDEX(virtual_address)] &
            TABLE_ADDRESS_MASK
        );

    if (!(pd[PD_INDEX(virtual_address)] & VMM_PRESENT))
        return -1;

    uint64_t *pt =
        (uint64_t *)(
            pd[PD_INDEX(virtual_address)] &
            TABLE_ADDRESS_MASK
        );

    uint64_t index = PT_INDEX(virtual_address);

    if (!(pt[index] & VMM_PRESENT))
        return -1;

    /*
     * Remove only the virtual-to-physical mapping.
     */
    pt[index] = 0;

    /*
     * Flush the stale translation from the TLB.
     */
    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory"
    );

    return 0;
}

/*
 * Return the PTE belonging to a virtual address.
 *
 * Returns 0 if any required page-table level does not exist.
 */
uint64_t vmm_get_pte(uint64_t *pml4,uint64_t virtual_address){
    if (!(pml4[PML4_INDEX(virtual_address)] & VMM_PRESENT))
        return 0;

    uint64_t *pdpt =
        (uint64_t *)(
            pml4[PML4_INDEX(virtual_address)] &
            TABLE_ADDRESS_MASK
        );

    if (!(pdpt[PDPT_INDEX(virtual_address)] & VMM_PRESENT))
        return 0;

    uint64_t *pd =
        (uint64_t *)(
            pdpt[PDPT_INDEX(virtual_address)] &
            TABLE_ADDRESS_MASK
        );

    if (!(pd[PD_INDEX(virtual_address)] & VMM_PRESENT))
        return 0;

    uint64_t *pt =
        (uint64_t *)(
            pd[PD_INDEX(virtual_address)] &
            TABLE_ADDRESS_MASK
        );

    return pt[PT_INDEX(virtual_address)];
}