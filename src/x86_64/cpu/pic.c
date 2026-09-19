#include "pic.h"
#include "../io.h"

#define PIC1_COMMAND 0x20
#define PIC1_DATA    0x21
#define PIC2_COMMAND 0xA0
#define PIC2_DATA    0xA1
#define PIC_EOI      0x20

void pic_remap(void){
    unsigned char master_mask = inb(PIC1_DATA);
    unsigned char slave_mask  = inb(PIC2_DATA);

    outb(PIC1_COMMAND, 0x11);
    outb(PIC2_COMMAND, 0x11);
    outb(PIC1_DATA, 0x20);
    outb(PIC2_DATA, 0x28);
    outb(PIC1_DATA, 0x04);
    outb(PIC2_DATA, 0x02);
    outb(PIC1_DATA, 0x01);
    outb(PIC2_DATA, 0x01);

    /*
     * IRQ0 = timer enabled
     * IRQ1 = keyboard masked
     */
    outb(PIC1_DATA, 0xFE);
    outb(PIC2_DATA, 0xFF);

    (void)master_mask;
    (void)slave_mask;
}

void pic_send_eoi(unsigned char irq){
    if (irq >= 8)
        outb(PIC2_COMMAND, PIC_EOI);

    outb(PIC1_COMMAND, PIC_EOI);
}

void pic_mask_irq(unsigned char irq){
    unsigned short port;
    unsigned char value;

    if (irq < 8)
        port = PIC1_DATA;
    else{
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port);
    value |= (1 << irq);
    outb(port, value);
}

void pic_unmask_irq(unsigned char irq){
    unsigned short port;
    unsigned char value;

    if (irq < 8)
        port = PIC1_DATA;
    else{
        port = PIC2_DATA;
        irq -= 8;
    }

    value = inb(port);
    value &= ~(1 << irq);
    outb(port, value);
}