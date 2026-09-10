#include "serial.h"

void kernel_main(){
    serial_init();
    serial_write_str("Hello, World!\n");
    serial_write_hex(0xDEADBEEFCAFEBABE);
    serial_write_str("\n");
}