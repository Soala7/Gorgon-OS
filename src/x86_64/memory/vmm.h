#ifndef VMM_H
#define VMM_H

#include <stdint.h>

#define VMM_PRESENT   (1ULL << 0)
#define VMM_WRITABLE  (1ULL << 1)
#define VMM_USER      (1ULL << 2)
#define VMM_NX        (1ULL << 63)

#define VMM_PHYSMAP_BASE 0xFFFF800000000000ULL

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

void vmm_init(uint64_t *bootstrap_pml4);

void vmm_switch_address_space(uint64_t *pml4);

uint64_t *vmm_get_current_pml4(void);

#endif