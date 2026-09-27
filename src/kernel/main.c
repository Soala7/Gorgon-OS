#include <stdint.h>

#include "serial.h"

#include "../x86_64/memory/pmm.h"
#include "../x86_64/memory/vmm.h"
#include "../x86_64/memory/heap.h"

#include "../x86_64/cpu/pic.h"
#include "../x86_64/cpu/timer.h"
#include "../x86_64/cpu/tss.h"

/*
 * Multiboot information address stored by the 32-bit boot code.
 *
 * main.asm:
 *
 *     multiboot_info:
 *         resd 1
 */
extern uint32_t multiboot_info;

/*
 * Bootstrap PML4 created by main.asm.
 *
 * It currently identity-maps the first 1 GiB using 2 MiB pages.
 */
extern uint64_t page_table_l4[];

void kernel_main(void)
{
    tss_init();

    serial_init();

    pic_remap();

    /*
     * Physical Memory Manager
     */
    pmm_init(multiboot_info);

    serial_write_str("Testing PMM...\n");

    uint64_t free_before = pmm_get_free_pages();

    uint64_t test_page = pmm_alloc_page();

    if (test_page != 0)
    {
        serial_write_str("Allocated page: ");
        serial_write_hex(test_page);
        serial_write_str("\n");

        serial_write_str("Free pages after allocation: ");
        serial_write_hex(pmm_get_free_pages());
        serial_write_str("\n");

        if (pmm_get_free_pages() == free_before - 1)
        {
            serial_write_str("PMM alloc count: PASS\n");
        }
        else
        {
            serial_write_str("PMM alloc count: FAIL\n");
        }

        pmm_free_page(test_page);

        serial_write_str("Freed page: ");
        serial_write_hex(test_page);
        serial_write_str("\n");

        serial_write_str("Free pages after freeing: ");
        serial_write_hex(pmm_get_free_pages());
        serial_write_str("\n");

        if (pmm_get_free_pages() == free_before)
        {
            serial_write_str("PMM free count: PASS\n");
        }
        else
        {
            serial_write_str("PMM free count: FAIL\n");
        }
    }
    else
    {
        serial_write_str("PMM allocation failed.\n");
    }

    serial_write_str("PMM test complete.\n");

    /*
     * Virtual Memory Manager
     *
     * vmm_init() currently:
     *
     *   1. Uses the bootstrap PML4.
     *   2. Tests 4 KiB mapping.
     *   3. Tests read/write.
     *   4. Tests PTE creation.
     *   5. Tests unmap.
     *   6. Tests PTE clearing.
     */
    vmm_init(page_table_l4);

    kmalloc_init();

    serial_write_str("Testing kernel heap...\n");

    void *a = kmalloc(16);
    void *b = kmalloc(64);
    void *c = kmalloc(4096);

    if (a && b && c){
        serial_write_str("kmalloc: PASS\n");
    }
    else{
        serial_write_str("kmalloc: FAIL\n");
    }

    kfree(b);

    void *d = kmalloc(32);

    if (d == b){
        serial_write_str("kfree/reuse: PASS\n");
    }
    else{
        serial_write_str("kfree/reuse: FAIL\n");
    }

    kfree(a);
    kfree(c);
    kfree(d);

    serial_write_str("Kernel heap test complete.\n");

    timer_init();

    serial_write_str("PIC and PIT initialized.\n");

    serial_write_str("Interrupts enabled.\n");

    __asm__ volatile("sti");

    uint64_t last_tick = 0;

    for (;;)
    {
        __asm__ volatile("hlt");

        uint64_t current_ticks = timer_get_ticks();

        if (current_ticks - last_tick >= 100)
        {
            last_tick = current_ticks;

            serial_write_str("Timer ticks: ");
            serial_write_dec(current_ticks);
            serial_write_str("\n");
        }
    }
}