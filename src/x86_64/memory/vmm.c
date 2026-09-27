#include "vmm.h"

#include "pmm.h"

#include "../../kernel/serial.h"

#include <stdint.h>

#define PML4_INDEX(addr) (((addr) >> 39) & 0x1FF)
#define PDPT_INDEX(addr) (((addr) >> 30) & 0x1FF)
#define PD_INDEX(addr) (((addr) >> 21) & 0x1FF)
#define PT_INDEX(addr) (((addr) >> 12) & 0x1FF)

#define TABLE_ADDRESS_MASK 0x000FFFFFFFFFF000ULL

#define VMM_PAGE_SIZE 0x1000ULL
#define VMM_HUGE_PAGE_SIZE 0x200000ULL
#define VMM_PAGE_SIZE_FLAG (1ULL << 7)

/*
 * The bootstrap page tables identity-map the first
 * 1 GiB using 2 MiB pages.
 *
 * M4B creates a new PML4 and copies the bootstrap
 * mappings into it before switching CR3.
 *
 * This gives Gorgon its own PML4 root while preserving
 * the mappings required by the currently running kernel.
 */
static uint64_t *current_pml4 = 0;
/*
 * Zero a newly allocated page-table page.
 */
static void zero_table(uint64_t *table)
{
    for (int i = 0; i < 512; i++)
    {
        table[i] = 0;
    }
}

/*
 * Switch CR3 to a new PML4.
 *
 * During the current identity-mapped environment,
 * the physical address of the PML4 can be accessed
 * directly as a pointer.
 */
void vmm_switch_address_space(uint64_t *pml4)
{
    current_pml4 = pml4;

    __asm__ volatile(
        "mov %0, %%cr3"
        :
        : "r"(pml4)
        : "memory"
    );
}

/*
 * Allocate and create a normal page-table entry.
 *
 * This is used for:
 *
 *     PML4 -> PDPT
 *     PDPT -> PD
 */
static uint64_t *get_or_create_table(
    uint64_t *table,
    uint64_t index,
    uint64_t flags)
{
    if (!(table[index] & VMM_PRESENT))
    {
        uint64_t new_table_phys = pmm_alloc_page();

        if (new_table_phys == 0)
        {
            return 0;
        }

        /*
         * The current address space identity-maps physical
         * memory, so the physical address can temporarily
         * be used directly as a virtual address.
         */
        uint64_t *new_table = (uint64_t *)new_table_phys;

        zero_table(new_table);

        table[index] =
            new_table_phys |
            VMM_PRESENT |
            VMM_WRITABLE |
            (flags & VMM_USER);
    }

    return (uint64_t *)(table[index] & TABLE_ADDRESS_MASK);
}

uint64_t *vmm_get_current_pml4(void)
{
    return current_pml4;
}
/*
 * Split an existing 2 MiB PDE into a normal 4 KiB page table.
 *
 * Example:
 *
 *     PDE = 2 MiB physical mapping + PS
 *
 * becomes:
 *
 *     PDE -> new PT
 *
 * where the 512 PTEs reproduce the original 2 MiB
 * mapping one 4 KiB page at a time.
 */
static uint64_t *split_large_page(
    uint64_t *pd,
    uint64_t index)
{
    uint64_t old_entry = pd[index];

    if (!(old_entry & VMM_PRESENT))
    {
        return 0;
    }

    if (!(old_entry & VMM_PAGE_SIZE_FLAG))
    {
        return (uint64_t *)(old_entry & TABLE_ADDRESS_MASK);
    }

    uint64_t new_pt_phys = pmm_alloc_page();

    if (new_pt_phys == 0)
    {
        return 0;
    }

    uint64_t *new_pt = (uint64_t *)new_pt_phys;

    zero_table(new_pt);

    uint64_t base = old_entry & 0x000FFFFFFFE00000ULL;

    /*
     * Preserve the useful permission bits from the
     * original 2 MiB mapping.
     */
    uint64_t pte_flags =
        old_entry &
        (VMM_PRESENT |
         VMM_WRITABLE |
         VMM_USER |
         VMM_NX);

    for (uint64_t i = 0; i < 512; i++)
    {
        new_pt[i] =
            (base + (i * VMM_PAGE_SIZE)) |
            pte_flags;
    }

    /*
     * Replace the huge-page PDE with a pointer
     * to the new PT.
     */
    pd[index] =
        new_pt_phys |
        VMM_PRESENT |
        VMM_WRITABLE |
        (old_entry & VMM_USER);

    return new_pt;
}

/*
 * Obtain the page table responsible for a virtual address.
 *
 * If the PDE is currently a 2 MiB mapping, split it into
 * 4 KiB pages first.
 */
static uint64_t *get_page_table(
    uint64_t *pml4,
    uint64_t virtual_address,
    uint64_t flags)
{
    uint64_t *pdpt =
        get_or_create_table(
            pml4,
            PML4_INDEX(virtual_address),
            flags);

    if (!pdpt)
    {
        return 0;
    }

    uint64_t *pd =
        get_or_create_table(
            pdpt,
            PDPT_INDEX(virtual_address),
            flags);

    if (!pd)
    {
        return 0;
    }

    uint64_t pd_index = PD_INDEX(virtual_address);

    /*
     * No PDE exists yet.
     */
    if (!(pd[pd_index] & VMM_PRESENT))
    {
        uint64_t new_pt_phys = pmm_alloc_page();

        if (new_pt_phys == 0)
        {
            return 0;
        }

        uint64_t *new_pt = (uint64_t *)new_pt_phys;

        zero_table(new_pt);

        pd[pd_index] =
            new_pt_phys |
            VMM_PRESENT |
            VMM_WRITABLE |
            (flags & VMM_USER);
    }

    /*
     * Existing PDE is a 2 MiB page.
     */
    if (pd[pd_index] & VMM_PAGE_SIZE_FLAG)
    {
        return split_large_page(pd, pd_index);
    }

    return (uint64_t *)(pd[pd_index] & TABLE_ADDRESS_MASK);
}

/*
 * Map one 4 KiB virtual page to one 4 KiB physical page.
 */
int vmm_map(
    uint64_t *pml4,
    uint64_t virtual_address,
    uint64_t physical_address,
    uint64_t flags)
{
    if ((virtual_address & (VMM_PAGE_SIZE - 1)) != 0)
    {
        return -1;
    }

    if ((physical_address & (VMM_PAGE_SIZE - 1)) != 0)
    {
        return -1;
    }

    uint64_t *pt =
        get_page_table(
            pml4,
            virtual_address,
            flags);

    if (!pt)
    {
        return -1;
    }

    uint64_t index = PT_INDEX(virtual_address);

    pt[index] =
        (physical_address & TABLE_ADDRESS_MASK) |
        (flags & (VMM_WRITABLE | VMM_USER | VMM_NX)) |
        VMM_PRESENT;

    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory");

    return 0;
}

/*
 * Remove a 4 KiB mapping.
 *
 * The physical frame is not freed here.
 */
int vmm_unmap(
    uint64_t *pml4,
    uint64_t virtual_address)
{
    if ((virtual_address & (VMM_PAGE_SIZE - 1)) != 0)
    {
        return -1;
    }

    uint64_t pml4e =
        pml4[PML4_INDEX(virtual_address)];

    if (!(pml4e & VMM_PRESENT))
    {
        return -1;
    }

    uint64_t *pdpt =
        (uint64_t *)(pml4e & TABLE_ADDRESS_MASK);

    uint64_t pdpte =
        pdpt[PDPT_INDEX(virtual_address)];

    if (!(pdpte & VMM_PRESENT))
    {
        return -1;
    }

    uint64_t *pd =
        (uint64_t *)(pdpte & TABLE_ADDRESS_MASK);

    uint64_t pde =
        pd[PD_INDEX(virtual_address)];

    if (!(pde & VMM_PRESENT))
    {
        return -1;
    }

    /*
     * We only unmap 4 KiB pages.
     *
     * Never accidentally treat a 2 MiB mapping
     * as a PT.
     */
    if (pde & VMM_PAGE_SIZE_FLAG)
    {
        return -1;
    }

    uint64_t *pt =
        (uint64_t *)(pde & TABLE_ADDRESS_MASK);

    uint64_t index = PT_INDEX(virtual_address);

    if (!(pt[index] & VMM_PRESENT))
    {
        return -1;
    }

    pt[index] = 0;

    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(virtual_address)
        : "memory");

    return 0;
}

/*
 * Return the PTE for a virtual address.
 *
 * Returns 0 if no normal 4 KiB PTE exists.
 */
uint64_t vmm_get_pte(
    uint64_t *pml4,
    uint64_t virtual_address)
{
    uint64_t pml4e =
        pml4[PML4_INDEX(virtual_address)];

    if (!(pml4e & VMM_PRESENT))
    {
        return 0;
    }

    uint64_t *pdpt =
        (uint64_t *)(pml4e & TABLE_ADDRESS_MASK);

    uint64_t pdpte =
        pdpt[PDPT_INDEX(virtual_address)];

    if (!(pdpte & VMM_PRESENT))
    {
        return 0;
    }

    /*
     * A 1 GiB PDPTE is not a normal PT hierarchy.
     */
    if (pdpte & VMM_PAGE_SIZE_FLAG)
    {
        return 0;
    }

    uint64_t *pd =
        (uint64_t *)(pdpte & TABLE_ADDRESS_MASK);

    uint64_t pde =
        pd[PD_INDEX(virtual_address)];

    if (!(pde & VMM_PRESENT))
    {
        return 0;
    }

    /*
     * A 2 MiB PDE is not a normal PTE.
     */
    if (pde & VMM_PAGE_SIZE_FLAG)
    {
        return 0;
    }

    uint64_t *pt =
        (uint64_t *)(pde & TABLE_ADDRESS_MASK);

    return pt[PT_INDEX(virtual_address)];
}

/*
 * M4B — Give Gorgon its own PML4.
 *
 * The bootstrap page tables already provide the mappings
 * required by the currently running kernel.
 *
 * We therefore:
 *
 *     1. Allocate a new PML4.
 *     2. Copy the bootstrap PML4 entries.
 *     3. Switch CR3 to the new PML4.
 *     4. Verify that the VMM still works.
 *
 * The lower-level page tables are temporarily shared with
 * the bootstrap address space. Full independent page-table
 * ownership can be introduced later when the address-space
 * manager is expanded.
 */
void vmm_init(uint64_t *bootstrap_pml4)
{
    serial_write_str("Initializing Gorgon address space...\n");

    /*
     * Allocate the new PML4.
     */
    uint64_t new_pml4_phys = pmm_alloc_page();

    if (new_pml4_phys == 0)
    {
        serial_write_str("VMM: failed to allocate new PML4.\n");
        return;
    }

    uint64_t *new_pml4 =
        (uint64_t *)new_pml4_phys;

    zero_table(new_pml4);

    /*
     * Copy the bootstrap PML4.
     *
     * Currently only the first PML4 entry is populated,
     * but copying all entries keeps this operation generic.
     */
    for (uint64_t i = 0; i < 512; i++)
    {
        new_pml4[i] = bootstrap_pml4[i];
    }

    serial_write_str("VMM: new PML4 allocated: ");
    serial_write_hex(new_pml4_phys);
    serial_write_str("\n");

    /*
     * Switch CR3.
     */
    vmm_switch_address_space(new_pml4);

    serial_write_str("VMM: switched to new address space.\n");

    /*
     * Test a normal 4 KiB mapping.
     */
    uint64_t virtual_address = 0x40000000ULL;

    uint64_t physical_address =
        pmm_alloc_page();

    if (physical_address == 0)
    {
        serial_write_str(
            "VMM test: physical allocation failed.\n");
        return;
    }

    if (vmm_map(
            new_pml4,
            virtual_address,
            physical_address,
            VMM_WRITABLE) != 0)
    {
        serial_write_str("VMM map: FAIL\n");

        pmm_free_page(physical_address);

        return;
    }

    serial_write_str("VMM map: PASS\n");

    /*
     * Test the actual virtual mapping.
     */
    volatile uint64_t *test_address =
        (volatile uint64_t *)virtual_address;

    *test_address = 0x123456789ABCDEF0ULL;

    if (*test_address == 0x123456789ABCDEF0ULL)
    {
        serial_write_str("VMM read/write: PASS\n");
    }
    else
    {
        serial_write_str("VMM read/write: FAIL\n");
    }

    /*
     * Confirm that a real 4 KiB PTE exists.
     */
    uint64_t pte =
        vmm_get_pte(
            new_pml4,
            virtual_address);

    if (pte & VMM_PRESENT)
    {
        serial_write_str("VMM PTE: PASS\n");
    }
    else
    {
        serial_write_str("VMM PTE: FAIL\n");
    }

    /*
     * Remove the mapping.
     */
    if (vmm_unmap(
            new_pml4,
            virtual_address) == 0)
    {
        serial_write_str("VMM unmap: PASS\n");
    }
    else
    {
        serial_write_str("VMM unmap: FAIL\n");
    }

    /*
     * Verify the PTE was cleared.
     */
    if (vmm_get_pte(
            new_pml4,
            virtual_address) == 0)
    {
        serial_write_str("VMM PTE cleared: PASS\n");
    }
    else
    {
        serial_write_str("VMM PTE cleared: FAIL\n");
    }

    /*
     * The physical page is no longer mapped,
     * so return it to the PMM.
     */
    pmm_free_page(physical_address);
}