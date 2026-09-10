#pragma once

#include <stdint.h>
#include <stddef.h>

#define COM1 0x3F8

void serial_init();
void serial_write(char c);
void serial_write_str(const char* str);
void serial_write_hex(uint64_t n);