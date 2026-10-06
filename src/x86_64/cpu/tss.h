#ifndef TSS_H
#define TSS_H

#include <stdint.h>

void tss_init(uint64_t gdt_tss_descriptor_phys);

#endif