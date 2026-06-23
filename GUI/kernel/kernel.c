/* VioletOS - Kernel entry point and initialization */
#include "violet.h"
#include "port.h"

static void reboot_system(void) {
    uint8_t good = 0x02;
    while (good & 0x02)
        good = inb(0x64);
    outb(0x64, 0xFE);
    for (;;) __asm__ volatile ("hlt");
}

static void shutdown_system(void) {
    outw(0x604, 0x2000); /* QEMU shutdown */
    outw(0xB004, 0x2000); /* Bochs shutdown */
    for (;;) __asm__ volatile ("hlt");
}

void kernel_panic(const char* message) {
    vga_set_color(0x0C, 0x00);
    vga_puts("\n\n*** KERNEL PANIC ***\n");
    vga_puts(message);
    vga_puts("\nSystem halted.\n");
    for (;;) __asm__ volatile ("cli; hlt");
}

void sys_reboot(void)  { reboot_system(); }
void sys_shutdown(void) { shutdown_system(); }

void kernel_main(uint32_t magic, struct multiboot_info* mb_info) {
    (void)magic;

    gdt_init();
    vga_init(mb_info);
    memory_init(mb_info);
    idt_init();
    pic_init();
    keyboard_init();
    timer_init(100);
    ata_init();
    fs_init();

    vga_clear();
    gui_show_logo();

    gui_main_menu();

    for (;;) __asm__ volatile ("hlt");
}
