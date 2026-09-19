#ifndef PIC_H
#define PIC_H

void pic_remap(void);
void pic_send_eoi(unsigned char irq);
void pic_mask_irq(unsigned char irq);
void pic_unmask_irq(unsigned char irq);

#endif