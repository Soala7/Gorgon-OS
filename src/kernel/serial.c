#include "serial.h"

// Write one byte to an x86 I/O port.
// This is an internal helper used by the serial driver.
static inline void outb(uint16_t port, uint8_t value){
    __asm__ volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}
// Read one byte from an x86 I/O port.
//  This is an internal helper used by the serial driver.
static inline uint8_t inb(uint16_t port){
    uint8_t value;
    __asm__ volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}
void serial_init(){
    // Serial initialization will go here.
    outb(COM1 + 3, 0x80); // Set DLAB = 1 (Divisor Latch Access Bit)
    outb(COM1 + 0, 0x01); // Baud divisor low byte
    outb(COM1 + 1, 0x00); // Baud divisor high byte
    outb(COM1 + 3, 0x03); // 8 data bits, no parity, 1 stop bit
    outb(COM1 + 2, 0xC7); // Enable FIFO, clear FIFO, 14-byte threshold
    outb(COM1 + 4, 0x0B); // Enable IRQs, RTS, and DTR
}

void serial_write(char c){
    while ((inb(COM1 + 5) & 0x20) == 0){   
        // Wait for the transmit buffer to be empty
    }
    outb(COM1 + 0, c);
    // Character output will go here.
}

void serial_write_str(const char *str){
    while (*str != '\0'){
        serial_write(*str);
        str++;
    }
    // String output will go here.
}

void serial_write_hex(uint64_t n){ //Convert the number to hexadecimal and write it to the serial port
    char hex[17]; // 16 hex digits + null terminator
    hex[16] = '\0'; // Null terminator
    for (int i = 15; i >= 0; i--){
        uint8_t digit = n & 0xF; // Get the last 4 bits
        if (digit < 10){
            hex[i] = '0' + digit; // Convert to ASCII '0'-'9'
        } else {
            hex[i] = 'A' + (digit - 10); // Convert to ASCII 'A'-'F'
        }
        n >>= 4; // Shift right by 4 bits to process the next digit
    }
    serial_write_str(hex);
    // Hexadecimal output will go here.
}