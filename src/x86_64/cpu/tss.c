#include "tss.h"
#include "gdt.h"
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

/*
 * gdt_tss_descriptor lives in the low bootstrap region.
 *
 * A normal C reference from the higher-half kernel would generate
 * an R_X86_64_PC32 relocation, which cannot reach the low bootstrap
 * address. The descriptor address is therefore obtained from the
 * bootstrap address-space mapping instead.
 *
 * The GDT bootstrap region is identity mapped during early boot, so
 * this physical address can be accessed directly while that mapping
 * still exists.
 */
#define GDT_TSS_DESCRIPTOR_PHYS 0x00100000ULL

static volatile uint8_t *gdt_tss_descriptor =
    (volatile uint8_t *)(uintptr_t)(GDT_TSS_DESCRIPTOR_PHYS + 0x2A);

static void gdt_set_tss_base(uint64_t base){
    gdt_tss_descriptor[2] = (uint8_t)(base & 0xFF);
    gdt_tss_descriptor[3] = (uint8_t)((base >> 8) & 0xFF);
    gdt_tss_descriptor[4] = (uint8_t)((base >> 16) & 0xFF);
    gdt_tss_descriptor[7] = (uint8_t)((base >> 24) & 0xFF);
    gdt_tss_descriptor[8] = (uint8_t)((base >> 32) & 0xFF);
    gdt_tss_descriptor[9] = (uint8_t)((base >> 40) & 0xFF);
    gdt_tss_descriptor[10] = (uint8_t)((base >> 48) & 0xFF);
    gdt_tss_descriptor[11] = (uint8_t)((base >> 56) & 0xFF);
}

void tss_init(void){
    kernel_tss.ist1 = (uint64_t)(df_stack + sizeof(df_stack));

    /*
     * Point the I/O permission bitmap base beyond the TSS limit
     * to disable it entirely - not using per-port I/O permissions.
     */
    kernel_tss.iomap_base = sizeof(tss_t);

    gdt_set_tss_base(
        (uint64_t)&kernel_tss
    );

    __asm__ volatile(
        "ltr %0"
        :
        : "r"((uint16_t)GDT_SELECTOR_TSS)
    );
}
