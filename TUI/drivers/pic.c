/* VioletOS - 8259 PIC remapping and IRQ control */
#include "violet.h"
#include "port.h"

static uint8_t master_mask = 0xFC;
static uint8_t slave_mask  = 0xFF;

void pic_init(void) {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    master_mask = 0xFC;
    slave_mask  = 0xFF;
    outb(0x21, master_mask);
    outb(0xA1, slave_mask);
}

void pic_eoi(uint8_t irq) {
    if (irq >= 8)
        outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

void pic_irq_enable(uint8_t irq) {
    if (irq < 8) {
        master_mask &= (uint8_t)~(1u << irq);
        outb(0x21, master_mask);
    } else {
        slave_mask &= (uint8_t)~(1u << (irq - 8));
        outb(0xA1, slave_mask);
    }
}

void pic_irq_disable(uint8_t irq) {
    if (irq < 8) {
        master_mask |= (uint8_t)(1u << irq);
        outb(0x21, master_mask);
    } else {
        slave_mask |= (uint8_t)(1u << (irq - 8));
        outb(0xA1, slave_mask);
    }
}
