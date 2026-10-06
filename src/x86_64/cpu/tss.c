#include "tss.h"
#include "gdt.h"
#include "../../kernel/serial.h"
#include <stdint.h>

#define TSS_IST1_STACK_SIZE 4096

typedef struct __attribute__((packed)){
    uint32_t reserved0;

    uint64_t rsp0;
    uint64_t rsp1;
    uint64_t rsp2;

    uint64_t reserved1;

    uint64_t ist1;
    uint64_t ist2;
    uint64_t ist3;
    uint64_t ist4;
    uint64_t ist5;
    uint64_t ist6;
    uint64_t ist7;

    uint64_t reserved2;

    uint16_t reserved3;
    uint16_t iomap_base;

} tss_t;

_Static_assert(
    sizeof(tss_t) == 104,
    "tss_t must be exactly 104 bytes - gdt_tss_descriptor's limit (0x67) assumes this"
);

/*
 * Dedicated stack for #DF (double fault), referenced via IST1.
 *
 * If a double fault is caused by kernel stack overflow/corruption,
 * running the handler on that same broken stack risks cascading
 * into an unrecoverable triple fault instead of a readable panic.
 */
static uint8_t df_stack[TSS_IST1_STACK_SIZE]
__attribute__((aligned(16)));

static tss_t kernel_tss;

static void gdt_set_tss_base(volatile uint8_t *descriptor, uint64_t base){
    descriptor[2]  = (uint8_t)(base & 0xFF);
    descriptor[3]  = (uint8_t)((base >> 8) & 0xFF);
    descriptor[4]  = (uint8_t)((base >> 16) & 0xFF);
    descriptor[7]  = (uint8_t)((base >> 24) & 0xFF);
    descriptor[8]  = (uint8_t)((base >> 32) & 0xFF);
    descriptor[9]  = (uint8_t)((base >> 40) & 0xFF);
    descriptor[10] = (uint8_t)((base >> 48) & 0xFF);
    descriptor[11] = (uint8_t)((base >> 56) & 0xFF);
}

/*
 * Read the base address back out of a patched TSS descriptor, so
 * tss_init() can prove to itself (and to the serial log) that the
 * patch actually landed, instead of just hoping ltr() didn't fault.
 */
static uint64_t gdt_read_tss_base(volatile uint8_t *descriptor){
    uint64_t base = 0;
    base |= (uint64_t)descriptor[2];
    base |= (uint64_t)descriptor[3]  << 8;
    base |= (uint64_t)descriptor[4]  << 16;
    base |= (uint64_t)descriptor[7]  << 24;
    base |= (uint64_t)descriptor[8]  << 32;
    base |= (uint64_t)descriptor[9]  << 40;
    base |= (uint64_t)descriptor[10] << 48;
    base |= (uint64_t)descriptor[11] << 56;
    return base;
}

/*
 * gdt_tss_descriptor_phys is the REAL physical address of
 * gdt_tss_descriptor (in gdt.asm's .boot.data), passed in from
 * main64.asm - which is still executing in the low bootstrap
 * region - via a plain absolute mov-immediate.
 *
 * This is deliberate, not an inconvenience to work around later:
 * a normal `extern` reference to that symbol from this high-half
 * translation unit cannot reach it. GCC's default RIP-relative
 * addressing for externs under -mcmodel=kernel emits an
 * R_X86_64_PC32 relocation, and the distance between the kernel's
 * high-half link address and the low bootstrap region exceeds what
 * a 32-bit PC-relative displacement can encode ("relocation
 * truncated to fit") - the same failure multiboot_info hit earlier
 * in the higher-half transition. Receiving the address as a plain
 * integer parameter sidesteps that entirely, with no hardcoded
 * offset anywhere in this file.
 */
void tss_init(uint64_t gdt_tss_descriptor_phys){
    volatile uint8_t *gdt_tss_descriptor = (volatile uint8_t *)(uintptr_t)gdt_tss_descriptor_phys;

    kernel_tss.ist1 = (uint64_t)(df_stack + sizeof(df_stack));

    /*
     * Point the I/O permission bitmap base beyond the TSS limit
     * to disable it entirely - not using per-port I/O permissions.
     */
    kernel_tss.iomap_base = sizeof(tss_t);

    gdt_set_tss_base(gdt_tss_descriptor, (uint64_t)&kernel_tss);

    __asm__ volatile(
        "ltr %0"
        :
        : "r"((uint16_t)GDT_SELECTOR_TSS)
    );

    uint64_t patched_base  = gdt_read_tss_base(gdt_tss_descriptor);
    uint64_t expected_base = (uint64_t)&kernel_tss;

    serial_write_str("TSS descriptor: ");
    serial_write_hex(gdt_tss_descriptor_phys);
    serial_write_str("\n");

    serial_write_str("TSS base:       ");
    serial_write_hex(patched_base);
    serial_write_str("\n");

    serial_write_str("TSS expected:   ");
    serial_write_hex(expected_base);
    serial_write_str("\n");

    if (patched_base == expected_base){
        serial_write_str("TSS: PASS\n");
    }else{
        serial_write_str("TSS: FAIL\n");
    }
}
