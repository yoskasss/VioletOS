/* VioletOS - Main kernel header */
#ifndef VIOLET_H
#define VIOLET_H

#include "types.h"
#include "multiboot.h"

#define VIOLETOS_VERSION  "1.0.0"
#define KERNEL_VERSION    "1.0.0"

#define KERNEL_CODE_SEG 0x08
#define KERNEL_DATA_SEG 0x10

/* Kernel API */
void kernel_main(uint32_t magic, struct multiboot_info* mb_info);
void kernel_panic(const char* message);

/* Architecture */
void gdt_init(void);
void idt_init(void);
void memory_init(struct multiboot_info* mb_info);
void* kmalloc(size_t size);
void* kcalloc(size_t count, size_t size);
void  kfree(void* ptr);
size_t get_total_memory(void);
size_t get_used_memory(void);
size_t get_free_memory(void);

/* Drivers */
void vga_init(struct multiboot_info* mb_info);
void vga_set_window_title(const char* title);
void vga_clear(void);
void vga_set_color(uint8_t fg, uint8_t bg);
void vga_putchar(char c);
void vga_puts(const char* str);
void vga_puthex(uint32_t n);
void vga_set_cursor(int row, int col);
void vga_get_cursor(int* row, int* col);
int  vga_get_rows(void);
int  vga_get_cols(void);
void vga_scroll(void);
void vga_putchar_at(int row, int col, char c, uint8_t fg, uint8_t bg);
uint16_t vga_get_cell(int row, int col);
void vga_hide_hw_cursor(void);
void vga_show_hw_cursor(void);

void keyboard_init(void);
void keyboard_push_char(char c);
char keyboard_getchar(void);
bool keyboard_has_char(void);
bool keyboard_is_shift(void);
bool keyboard_is_caps(void);
bool keyboard_poll(char* c);
uint8_t keyboard_get_scancode(void);
void keyboard_flush(void);
void keyboard_wait_release(void);
bool keyboard_key_down(uint8_t sc);
bool keyboard_ext_down(uint8_t sc);

void sys_reboot(void);
void sys_shutdown(void);
const char* fs_get_cwd(void);
int fs_set_cwd(const char* path);

#define FS_MAX_NAME 32

#define KEY_UP    0xF0
#define KEY_DOWN  0xF1
#define KEY_LEFT  0xF2
#define KEY_RIGHT 0xF3
#define KEY_ENTER 0x0A
#define KEY_BS    0x08

void timer_init(uint32_t frequency);
void pic_init(void);
void pic_eoi(uint8_t irq);
void pic_irq_enable(uint8_t irq);
void pic_irq_disable(uint8_t irq);
uint32_t timer_get_ticks(void);
uint32_t timer_get_uptime_seconds(void);

void ata_init(void);
bool ata_read_sector(uint32_t lba, uint8_t* buffer);
bool ata_write_sector(uint32_t lba, const uint8_t* buffer);

void vga_mode13_init(void);
void vga_mode13_clear(uint8_t color);
void vga_mode13_putpixel(int x, int y, uint8_t color);
void vga_mode13_fill_rect(int x, int y, int w, int h, uint8_t color);
void vga_mode13_draw_rect(int x, int y, int w, int h, uint8_t color);
void vga_text_restore(void);

void timer_handler(void);
void keyboard_handler(void);

/* Filesystem */
void fs_init(void);
bool fs_save(void);
bool fs_load(void);
int  fs_mkdir(const char* path);
int  fs_touch(const char* path);
int  fs_rm(const char* path);
int  fs_cat(const char* path, char* buf, size_t buf_size);
int  fs_write(const char* path, const char* data, size_t len);
int  fs_read(const char* path, char* buf, size_t buf_size);
int  fs_ls(const char* path, char* buf, size_t buf_size);
int  fs_exists(const char* path);
size_t fs_get_used_storage(void);
size_t fs_get_total_storage(void);

/* GUI */
void gui_show_logo(void);
void gui_main_menu(void);

/* Shell */
void shell_run(void);

/* IDE */
void ide_run(void);

/* Lib */
void* memset(void* s, int c, size_t n);
void* memcpy(void* dest, const void* src, size_t n);
void* memmove(void* dest, const void* src, size_t n);
int   memcmp(const void* s1, const void* s2, size_t n);
size_t strlen(const char* s);
char* strcpy(char* dest, const char* src);
char* strncpy(char* dest, const char* src, size_t n);
int   strcmp(const char* s1, const char* s2);
int   strncmp(const char* s1, const char* s2, size_t n);
char* strcat(char* dest, const char* src);
char* strchr(const char* s, int c);
char* strstr(const char* haystack, const char* needle);
int   atoi(const char* s);
char* itoa(int value, char* str, int base);
void  delay_ms(uint32_t ms);

#endif /* VIOLET_H */
