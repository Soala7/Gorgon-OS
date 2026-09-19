#ifndef GDT_H
#define GDT_H

#define GDT_SELECTOR_NULL 0x00
#define GDT_SELECTOR_CODE 0x08
#define GDT_SELECTOR_DATA 0x10
#define GDT_SELECTOR_TSS  0x18

void gdt_load(void);

#endif