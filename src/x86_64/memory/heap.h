#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>

void kmalloc_init(void);

void *kmalloc(uint64_t size);

void kfree(void *ptr);

#endif