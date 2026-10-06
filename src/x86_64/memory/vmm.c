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
 * current_pml4 is a physmap VIRTUAL pointer, for C-side table
 * walks. CR3 always gets the raw PHYSICAL value separately - the
 * two are never the same number once phys_to_virt() is involved,
 * and must not be conflated.
 */
static uint64_t *current_pml4 = 0;
static uint64_t current_pml4_phys = 0;

static void zero_table(uint64_t *table){
    for (int i = 0; i < 512; i++)
        table[i] = 0;
}

static inline uint64_t *phys_to_virt(uint64_t physical_address){
    return (uint64_t *)(VMM_PHYSMAP_BASE + physical_address);
}

/*
 * Takes the PHYSICAL address of a PML4. Loads CR3 with it directly,
 * and stores the physmap-translated pointer for C-side use.
 */
void vmm_switch_address_space(uint64_t pml4_phys){
    current_pml4_phys = pml4_phys;
    current_pml4 = phys_to_virt(pml4_phys);

    __asm__ volatile(
        "mov %0, %%cr3"
        :
        : "r"(pml4_phys)
        : "memory");
}

uint64_t *vmm_get_current_pml4(void){
    return current_pml4;
}

uint64_t vmm_get_current_pml4_phys(void){
    return current_pml4_phys;
}

/*
 * Split a 2 MiB PDE into a 4 KiB page table, preserving its
 * permission bits across all 512 new entries.
 */
static uint64_t *split_large_page(uint64_t *pd, uint64_t index){
    uint64_t old_entry = pd[index];

    if (!(old_entry & VMM_PRESENT))
        return 0;

    if (!(old_entry & VMM_PAGE_SIZE_FLAG))
        return (uint64_t *)(old_entry & TABLE_ADDRESS_MASK);

    uint64_t new_pt_phys = pmm_alloc_page();

    if (new_pt_phys == 0)
        return 0;

    uint64_t *new_pt = phys_to_virt(new_pt_phys);
    zero_table(new_pt);

    uint64_t base = old_entry & 0x000FFFFFFFE00000ULL;
    uint64_t pte_flags = old_entry & (VMM_PRESENT | VMM_WRITABLE | VMM_USER | VMM_NX);

    for (uint64_t i = 0; i < 512; i++)
        new_pt[i] = (base + (i * VMM_PAGE_SIZE)) | pte_flags;

    pd[index] = new_pt_phys | VMM_PRESENT | VMM_WRITABLE | (old_entry & VMM_USER);

    return new_pt;
}

static uint64_t *get_or_create_table(uint64_t *table, uint64_t index, uint64_t flags){
    if (!(table[index] & VMM_PRESENT)){
        uint64_t new_table_phys = pmm_alloc_page();

        if (new_table_phys == 0)
            return 0;

        uint64_t *new_table = phys_to_virt(new_table_phys);
        zero_table(new_table);

        table[index] = new_table_phys | VMM_PRESENT | VMM_WRITABLE | (flags & VMM_USER);
    }

    return phys_to_virt(table[index] & TABLE_ADDRESS_MASK);
}

/*
 * Temporary bootstrap physmap: VA 0xFFFF800000000000 -> PA 0,
 * 1 GiB, 2 MiB pages (PML4[256], matches the current PMM 1 GiB cap).
 *
 * Runs before the CR3 switch, so pml4/pdpt/pd here are accessed as
 * raw identity-mapped physical pointers - valid only because they're
 * all within the bootstrap's existing low-memory identity map.
 */
static int vmm_map_physmap(uint64_t *pml4){
    uint64_t pdpt_phys = pmm_alloc_page();
    uint64_t pd_phys = pmm_alloc_page();

    if (pdpt_phys == 0 || pd_phys == 0)
        return -1;

    uint64_t *pdpt = (uint64_t *)pdpt_phys;
    uint64_t *pd = (uint64_t *)pd_phys;

    for (int i = 0; i < 512; i++)
        pdpt[i] = 0;

    for (int i = 0; i < 512; i++)
        pd[i] = 0;

    pml4[256] = pdpt_phys | VMM_PRESENT | VMM_WRITABLE;
    pdpt[0] = pd_phys | VMM_PRESENT | VMM_WRITABLE;

    for (uint64_t i = 0; i < 512; i++)
        pd[i] = (i * VMM_HUGE_PAGE_SIZE) | VMM_PRESENT | VMM_WRITABLE | VMM_PAGE_SIZE_FLAG;

    return 0;
}

/*
 * Resolve the PT for a virtual address, splitting a 2 MiB PDE into
 * 4 KiB pages first if needed.
 */
static uint64_t *get_page_table(uint64_t *pml4, uint64_t virtual_address, uint64_t flags){
    uint64_t *pdpt = get_or_create_table(pml4, PML4_INDEX(virtual_address), flags);

    if (!pdpt)
        return 0;

    uint64_t *pd = get_or_create_table(pdpt, PDPT_INDEX(virtual_address), flags);

    if (!pd)
        return 0;

    uint64_t pd_index = PD_INDEX(virtual_address);

    if (!(pd[pd_index] & VMM_PRESENT)){
        uint64_t new_pt_phys = pmm_alloc_page();

        if (new_pt_phys == 0)
            return 0;

        uint64_t *new_pt = phys_to_virt(new_pt_phys);
        zero_table(new_pt);

        pd[pd_index] = new_pt_phys | VMM_PRESENT | VMM_WRITABLE | (flags & VMM_USER);
    }

    if (pd[pd_index] & VMM_PAGE_SIZE_FLAG)
        return split_large_page(pd, pd_index);

    return phys_to_virt(pd[pd_index] & TABLE_ADDRESS_MASK);
}

int vmm_map(uint64_t *pml4, uint64_t virtual_address, uint64_t physical_address, uint64_t flags){
    if ((virtual_address & (VMM_PAGE_SIZE - 1)) != 0)
        return -1;

    if ((physical_address & (VMM_PAGE_SIZE - 1)) != 0)
        return -1;

    uint64_t *pt = get_page_table(pml4, virtual_address, flags);

    if (!pt)
        return -1;

    uint64_t index = PT_INDEX(virtual_address);

    pt[index] = (physical_address & TABLE_ADDRESS_MASK) |
                (flags & (VMM_WRITABLE | VMM_USER | VMM_NX)) |
                VMM_PRESENT;

    __asm__ volatile("invlpg (%0)" : : "r"(virtual_address) : "memory");

    return 0;
}

/*
 * Removes the mapping only. The physical frame is not freed here.
 */
int vmm_unmap(uint64_t *pml4, uint64_t virtual_address){
    if ((virtual_address & (VMM_PAGE_SIZE - 1)) != 0)
        return -1;

    uint64_t pml4e = pml4[PML4_INDEX(virtual_address)];

    if (!(pml4e & VMM_PRESENT))
        return -1;

    uint64_t *pdpt = phys_to_virt(pml4e & TABLE_ADDRESS_MASK);
    uint64_t pdpte = pdpt[PDPT_INDEX(virtual_address)];

    if (!(pdpte & VMM_PRESENT))
        return -1;

    uint64_t *pd = phys_to_virt(pdpte & TABLE_ADDRESS_MASK);
    uint64_t pde = pd[PD_INDEX(virtual_address)];

    if (!(pde & VMM_PRESENT))
        return -1;

    /* Never treat a 2 MiB mapping as a 4 KiB PT. */
    if (pde & VMM_PAGE_SIZE_FLAG)
        return -1;

    uint64_t *pt = phys_to_virt(pde & TABLE_ADDRESS_MASK);
    uint64_t index = PT_INDEX(virtual_address);

    if (!(pt[index] & VMM_PRESENT))
        return -1;

    pt[index] = 0;

    __asm__ volatile("invlpg (%0)" : : "r"(virtual_address) : "memory");

    return 0;
}

/*
 * Returns 0 if no normal 4 KiB PTE exists (missing level, or a
 * 2 MiB PDE still in place).
 */
uint64_t vmm_get_pte(uint64_t *pml4, uint64_t virtual_address){
    uint64_t pml4e = pml4[PML4_INDEX(virtual_address)];

    if (!(pml4e & VMM_PRESENT))
        return 0;

    uint64_t *pdpt = phys_to_virt(pml4e & TABLE_ADDRESS_MASK);
    uint64_t pdpte = pdpt[PDPT_INDEX(virtual_address)];

    if (!(pdpte & VMM_PRESENT) || (pdpte & VMM_PAGE_SIZE_FLAG))
        return 0;

    uint64_t *pd = phys_to_virt(pdpte & TABLE_ADDRESS_MASK);
    uint64_t pde = pd[PD_INDEX(virtual_address)];

    if (!(pde & VMM_PRESENT) || (pde & VMM_PAGE_SIZE_FLAG))
        return 0;

    uint64_t *pt = phys_to_virt(pde & TABLE_ADDRESS_MASK);
    return pt[PT_INDEX(virtual_address)];
}

/*
 * M4B - give Gorgon its own PML4:
 *   1. Allocate + zero a new PML4 (built via raw identity-mapped
 *      access - physmap isn't live yet at this point).
 *   2. Copy the bootstrap PML4 entries into it.
 *   3. Build the temporary 1 GiB physmap inside it.
 *   4. Switch CR3 - physmap is live from here on.
 *   5. From here on, address the PML4 through phys_to_virt(),
 *      never through the raw identity-mapped pointer - that
 *      pointer only keeps working while the identity map exists,
 *      and removing it is the end goal.
 */
void vmm_init(uint64_t *bootstrap_pml4){
    serial_write_str("Initializing Gorgon address space...\n");

    uint64_t new_pml4_phys = pmm_alloc_page();

    if (new_pml4_phys == 0){
        serial_write_str("VMM: failed to allocate new PML4.\n");
        return;
    }

    uint64_t *new_pml4 = (uint64_t *)new_pml4_phys;
    zero_table(new_pml4);

    serial_write_str("VMM: new PML4 allocated: ");
    serial_write_hex(new_pml4_phys);
    serial_write_str("\n");

    for (int i = 0; i < 512; i++)
        new_pml4[i] = bootstrap_pml4[i];

    if (vmm_map_physmap(new_pml4) != 0){
        serial_write_str("VMM: physmap creation FAILED\n");
        return;
    }

    serial_write_str("VMM: physmap created.\n");

    vmm_switch_address_space(new_pml4_phys);

    serial_write_str("VMM: switched to new address space.\n");

    /* All table access below goes through vmm_get_current_pml4()
     * (a physmap pointer), never the raw new_pml4/new_pml4_phys
     * used to build the table above. */
    uint64_t *pml4 = vmm_get_current_pml4();

    uint64_t physmap_test_phys = pmm_alloc_page();

    if (physmap_test_phys == 0){
        serial_write_str("VMM physmap test: allocation FAILED\n");
        return;
    }

    volatile uint64_t *physmap_test = (volatile uint64_t *)(VMM_PHYSMAP_BASE + physmap_test_phys);

    *physmap_test = 0xCAFEBABE12345678ULL;

    if (*physmap_test == 0xCAFEBABE12345678ULL)
        serial_write_str("VMM physmap read/write: PASS\n");
    else
        serial_write_str("VMM physmap read/write: FAIL\n");

    pmm_free_page(physmap_test_phys);

    uint64_t virtual_address = 0x40000000ULL;
    uint64_t physical_address = pmm_alloc_page();

    if (physical_address == 0){
        serial_write_str("VMM test: physical allocation failed.\n");
        return;
    }

    if (vmm_map(pml4, virtual_address, physical_address, VMM_WRITABLE) != 0){
        serial_write_str("VMM map: FAIL\n");
        pmm_free_page(physical_address);
        return;
    }

    serial_write_str("VMM map: PASS\n");

    volatile uint64_t *test_address = (volatile uint64_t *)virtual_address;
    *test_address = 0x123456789ABCDEF0ULL;

    if (*test_address == 0x123456789ABCDEF0ULL)
        serial_write_str("VMM read/write: PASS\n");
    else
        serial_write_str("VMM read/write: FAIL\n");

    uint64_t pte = vmm_get_pte(pml4, virtual_address);

    if (pte & VMM_PRESENT)
        serial_write_str("VMM PTE: PASS\n");
    else
        serial_write_str("VMM PTE: FAIL\n");

    if (vmm_unmap(pml4, virtual_address) == 0)
        serial_write_str("VMM unmap: PASS\n");
    else
        serial_write_str("VMM unmap: FAIL\n");

    if (vmm_get_pte(pml4, virtual_address) == 0)
        serial_write_str("VMM PTE cleared: PASS\n");
    else
        serial_write_str("VMM PTE cleared: FAIL\n");

    pmm_free_page(physical_address);

    /*
     * Deliberate #PF test (map/unmap/deref-anyway) confirmed working
     * on 2026-10-04: CR2 and error code both correct, handler halted
     * as expected. Removed here so boot continues past this point;
     * re-add if either the PML4/physmap handling or the page-fault
     * path changes again.
     */
}
