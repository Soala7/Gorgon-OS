#include <stdint.h>
#include "serial.h"
#include "../x86_64/memory/pmm.h"
#include "../x86_64/memory/vmm.h"
#include "../x86_64/cpu/pic.h"
#include "../x86_64/cpu/timer.h"
#include "../x86_64/cpu/tss.h"

// 1. Multiboot structure physical address pointer
extern uintptr_t multiboot_info;
extern uint64_t page_table_l4[];

// Inline TLB flush helper for single page
static inline void flush_tlb_page(uint64_t address){
    __asm__ volatile(
        "invlpg (%0)"
        :
        : "r"(address)
        : "memory"
    );
}

void kernel_main(void) {
    tss_init();
    serial_init();
    pic_remap();

    pmm_init(multiboot_info);

    serial_write_str("Testing PMM...\n");
    uint64_t free_before = pmm_get_free_pages();
    uint64_t test_page = pmm_alloc_page();

    if (test_page != 0) {
        serial_write_str("Allocated page: ");
        serial_write_hex(test_page);
        serial_write_str("\n");

        serial_write_str("Free pages after allocation: ");
        serial_write_hex(pmm_get_free_pages());
        serial_write_str("\n");

        if (pmm_get_free_pages() == free_before - 1) {
            serial_write_str("PMM alloc count: PASS\n");
        } else {
            serial_write_str("PMM alloc count: FAIL\n");
        }

        pmm_free_page(test_page);

        serial_write_str("Freed page: ");
        serial_write_hex(test_page);
        serial_write_str("\n");
        serial_write_str("Free pages after freeing: ");
        serial_write_hex(pmm_get_free_pages());
        serial_write_str("\n");

        if (pmm_get_free_pages() == free_before) {
            serial_write_str("PMM free count: PASS\n");
        } else {
            serial_write_str("PMM free count: FAIL\n");
        }
    } else {
        serial_write_str("PMM allocation failed.\n");
    }

    serial_write_str("PMM test complete.\n");

    serial_write_str("Testing VMM...\n");

    // 2. Select a virtual address suitable for your VMM scheme
    uint64_t virtual_address = 0x40000000ULL; 
    uint64_t physical_address = pmm_alloc_page();

    if (physical_address == 0) {
        serial_write_str("VMM test: physical allocation failed.\n");
    } else {
        int result = vmm_map(page_table_l4, virtual_address, physical_address, VMM_WRITABLE);

        if (result == 0) {
            serial_write_str("VMM map: PASS\n");

            volatile uint64_t *test_address = (volatile uint64_t *)virtual_address;
            *test_address = 0x123456789ABCDEF0ULL;

            if (*test_address == 0x123456789ABCDEF0ULL) {
                serial_write_str("VMM read/write: PASS\n");
            } else {
                serial_write_str("VMM read/write: FAIL\n");
            }

            uint64_t pte = vmm_get_pte(page_table_l4, virtual_address);

            if (pte & VMM_PRESENT) {
                serial_write_str("VMM PTE: PASS\n");
            } else {
                serial_write_str("VMM PTE: FAIL\n");
            }

            if (vmm_unmap(page_table_l4, virtual_address) == 0) {
                // 3. Flush TLB so CPU registers the unmap operation immediately
                flush_tlb_page(virtual_address);
                serial_write_str("VMM unmap: PASS\n");
            } else {
                serial_write_str("VMM unmap: FAIL\n");
            }

            if (vmm_get_pte(page_table_l4, virtual_address) == 0) {
                serial_write_str("VMM PTE cleared: PASS\n");
            } else {
                serial_write_str("VMM PTE cleared: FAIL\n");
            }
        } else {
            serial_write_str("VMM map: FAIL\n");
        }

        pmm_free_page(physical_address);
    }
    serial_write_str("Testing page fault...\n");

    volatile uint64_t *fault_address =(volatile uint64_t *)0x40000000ULL;

    (void)*fault_address;

    /*
    * Execution should never reach here because the
    * page-fault handler currently halts the CPU.
    */
    serial_write_str("Page Fault test FAILED: execution continued.\n");
    timer_init();
    serial_write_str("PIC and PIT initialized.\n");
    serial_write_str("Interrupts enabled.\n");
    __asm__ volatile("sti");

    unsigned long last_tick = 0;

    for (;;) {
        __asm__ volatile("hlt");

        uint64_t current_ticks = timer_get_ticks();
        if (current_ticks - last_tick >= 100) {
            last_tick = current_ticks;
            serial_write_str("Timer ticks: ");
            serial_write_hex(current_ticks);
            serial_write_str("\n");
        }
    }
}
