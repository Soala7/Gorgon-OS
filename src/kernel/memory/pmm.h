#ifndef PMM_H
#define PMM_H

#include <stdint.h>

#define PAGE_SIZE 4096

void pmm_init(uint32_t multiboot_info_address);
uint64_t pmm_alloc_page(void);
void pmm_free_page(uint64_t address);
uint64_t pmm_get_total_pages(void);
uint64_t pmm_get_free_pages(void);

#endif