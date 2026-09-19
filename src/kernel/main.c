#include "serial.h"
#include "task.h"
#include "../x86_64/memory/pmm.h"
#include "../x86_64/cpu/pic.h"
#include "../x86_64/cpu/timer.h"
#include "../x86_64/cpu/tss.h"

extern uint32_t multiboot_info;

void kernel_main(){
// FIX: must run before sti (interrupts enabled) further down -
// IST switching requires TR to already point at a valid TSS,
// or #DF's IST=1 setting in idt.asm has undefined behavior the
// moment a double fault actually happens.
tss_init();

serial_init();

// TEMP TEST (M2 exception verification) - REMOVE after confirming
// serial output shows "Exception vector: 0x0 Error code: 0x0" and
// the CPU halts cleanly instead of resetting/triple-faulting.
serial_write_str("Triggering deliberate #DE (divide by zero)...\n");
volatile int de_test_zero = 0;
volatile int de_test_result = 1 / de_test_zero;
(void)de_test_result;
// If you see this line, the exception did NOT halt the CPU -
// that itself would be a bug worth reporting back.
serial_write_str("If you see this, #DE did not halt as expected.\n");
// END TEMP TEST

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

// FIX: free_before was captured but never actually checked
// against anything, so this test could not fail even if
// alloc/free were broken. Now it verifies the count actually
// moves by exactly one page in each direction.
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
task_init();
task_t *test_task = task_create();
task_t *test_task_2 = task_create();

if (test_task != 0)
    {
serial_write_str("Test task created: ");
serial_write_hex(test_task->id);
serial_write_str("\n");
    }
if (test_task_2 != 0)
    {
serial_write_str("Test task created 2: ");
serial_write_hex(test_task_2->id);
serial_write_str("\n");
    }
serial_write_str("Kernel started.\n");

// FIX: pic_remap() was called a second time here, right before
// timer_init(). It's already been called once above, right
// after serial_init(). Harmless (idempotent) but redundant.
timer_init();
serial_write_str("PIC and PIT initialized.\n");
serial_write_str("Interrupts enabled.\n");
__asm__ volatile("sti");
unsigned long last_tick = 0;

for (;;)
    {
__asm__ volatile("hlt");
if (timer_get_ticks() - last_tick >= 100)
        {
last_tick = timer_get_ticks();

serial_write_str("Timer ticks: ");
serial_write_hex(timer_get_ticks());
serial_write_str("\n");
        }
    }
}