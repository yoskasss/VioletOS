/* VioletOS - Interrupt Descriptor Table */
#include "violet.h"
#include "port.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

static struct idt_entry idt[256];
static struct idt_ptr     idtp;

extern void isr0(void);  extern void isr1(void);  extern void isr2(void);
extern void isr3(void);  extern void isr4(void);  extern void isr5(void);
extern void isr6(void);  extern void isr7(void);  extern void isr8(void);
extern void isr9(void);  extern void isr10(void); extern void isr11(void);
extern void isr12(void); extern void isr13(void); extern void isr14(void);
extern void isr15(void); extern void isr16(void); extern void isr17(void);
extern void isr18(void); extern void isr19(void); extern void isr20(void);
extern void isr21(void); extern void isr22(void); extern void isr23(void);
extern void isr24(void); extern void isr25(void); extern void isr26(void);
extern void isr27(void); extern void isr28(void); extern void isr29(void);
extern void isr30(void); extern void isr31(void);
extern void isr32(void); extern void isr33(void); extern void isr34(void);
extern void isr35(void); extern void isr36(void); extern void isr37(void);
extern void isr38(void); extern void isr39(void); extern void isr40(void);
extern void isr41(void); extern void isr42(void); extern void isr43(void);
extern void isr44(void); extern void isr45(void); extern void isr46(void);
extern void isr47(void);
extern void idt_load(uint32_t ptr);

static void idt_set_gate(uint8_t num, uint32_t base, uint16_t sel, uint8_t flags) {
    idt[num].base_low  = base & 0xFFFF;
    idt[num].base_high = (base >> 16) & 0xFFFF;
    idt[num].sel       = sel;
    idt[num].always0   = 0;
    idt[num].flags     = flags;
}

static void idt_install_isr(uint8_t num, void (*handler)(void)) {
    idt_set_gate(num, (uint32_t)handler, 0x08, 0x8E);
}

void idt_init(void) {
    idtp.limit = (sizeof(struct idt_entry) * 256) - 1;
    idtp.base  = (uint32_t)&idt;
    memset(&idt, 0, sizeof(idt));

    idt_install_isr(0,  isr0);  idt_install_isr(1,  isr1);
    idt_install_isr(2,  isr2);  idt_install_isr(3,  isr3);
    idt_install_isr(4,  isr4);  idt_install_isr(5,  isr5);
    idt_install_isr(6,  isr6);  idt_install_isr(7,  isr7);
    idt_install_isr(8,  isr8);  idt_install_isr(9,  isr9);
    idt_install_isr(10, isr10); idt_install_isr(11, isr11);
    idt_install_isr(12, isr12); idt_install_isr(13, isr13);
    idt_install_isr(14, isr14); idt_install_isr(15, isr15);
    idt_install_isr(16, isr16); idt_install_isr(17, isr17);
    idt_install_isr(18, isr18); idt_install_isr(19, isr19);
    idt_install_isr(20, isr20); idt_install_isr(21, isr21);
    idt_install_isr(22, isr22); idt_install_isr(23, isr23);
    idt_install_isr(24, isr24); idt_install_isr(25, isr25);
    idt_install_isr(26, isr26); idt_install_isr(27, isr27);
    idt_install_isr(28, isr28); idt_install_isr(29, isr29);
    idt_install_isr(30, isr30); idt_install_isr(31, isr31);

    for (int i = 32; i <= 47; i++) {
        void (*handlers[])(void) = {
            isr32, isr33, isr34, isr35, isr36, isr37, isr38, isr39,
            isr40, isr41, isr42, isr43, isr44, isr45, isr46, isr47
        };
        idt_install_isr((uint8_t)i, handlers[i - 32]);
    }

    idt_load((uint32_t)&idtp);
}

void isr_handler(uint32_t exception) {
    static char msg[40];
    if (exception == 8) {
        kernel_panic("Double fault (stack/IRQ overflow)");
        return;
    }
    msg[0] = 'C'; msg[1] = 'P'; msg[2] = 'U'; msg[3] = ' ';
    msg[4] = 'E'; msg[5] = 'x'; msg[6] = 'c'; msg[7] = ' ';
    msg[8] = '#';
    char num[8];
    itoa((int)exception, num, 10);
    int i = 9, j = 0;
    while (num[j]) msg[i++] = num[j++];
    msg[i] = '\0';
    kernel_panic(msg);
}

void irq_dispatch(uint32_t int_no) {
    if (int_no == 32) {
        timer_handler();
        return;
    }
    if (int_no == 33) {
        keyboard_handler();
        return;
    }
    if (int_no >= 32 && int_no <= 47)
        pic_eoi((uint8_t)(int_no - 32));
}
