/* VioletOS - VGA text mode driver */
#include "violet.h"
#include "port.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25
#define VGA_MEMORY 0xB8000

enum vga_color {
    VGA_BLACK = 0,
    VGA_BLUE = 1,
    VGA_GREEN = 2,
    VGA_CYAN = 3,
    VGA_RED = 4,
    VGA_MAGENTA = 5,
    VGA_BROWN = 6,
    VGA_LIGHT_GREY = 7,
    VGA_DARK_GREY = 8,
    VGA_LIGHT_BLUE = 9,
    VGA_LIGHT_GREEN = 10,
    VGA_LIGHT_CYAN = 11,
    VGA_LIGHT_RED = 12,
    VGA_LIGHT_MAGENTA = 13,
    VGA_YELLOW = 14,
    VGA_WHITE = 15,
};

#define VGA_COLOR_PURPLE      VGA_MAGENTA
#define VGA_COLOR_LIGHT_PURPLE VGA_LIGHT_MAGENTA

static uint16_t* vga_buffer = (uint16_t*)VGA_MEMORY;
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t vga_color = 0;

static inline uint8_t vga_entry_color(uint8_t fg, uint8_t bg) {
    return fg | bg << 4;
}

static inline uint16_t vga_entry(unsigned char c, uint8_t color) {
    return (uint16_t)c | (uint16_t)color << 8;
}

void vga_init(void) {
    vga_set_color(VGA_COLOR_LIGHT_PURPLE, VGA_BLACK);
    vga_hide_hw_cursor();
    vga_clear();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color = vga_entry_color(fg, bg);
}

void vga_clear(void) {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_entry(' ', vga_color);
        }
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_set_cursor(0, 0);
}

void vga_set_cursor(int row, int col) {
    if (row < 0) row = 0;
    if (col < 0) col = 0;
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    if (col >= VGA_WIDTH) col = VGA_WIDTH - 1;
    cursor_row = row;
    cursor_col = col;
    uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

void vga_get_cursor(int* row, int* col) {
    if (row) *row = cursor_row;
    if (col) *col = cursor_col;
}

int vga_get_rows(void) { return VGA_HEIGHT; }
int vga_get_cols(void) { return VGA_WIDTH; }

void vga_putchar_at(int row, int col, char c, uint8_t fg, uint8_t bg) {
    if (row < 0 || col < 0 || row >= VGA_HEIGHT || col >= VGA_WIDTH) return;
    vga_buffer[row * VGA_WIDTH + col] = vga_entry((unsigned char)c, vga_entry_color(fg, bg));
}

uint16_t vga_get_cell(int row, int col) {
    if (row < 0 || col < 0 || row >= VGA_HEIGHT || col >= VGA_WIDTH) return vga_entry(' ', vga_color);
    return vga_buffer[row * VGA_WIDTH + col];
}

void vga_hide_hw_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

void vga_show_hw_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x0E);
    outb(0x3D4, 0x0B);
    outb(0x3D5, 0x0F);
}

void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y + 1) * VGA_WIDTH + x];
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = vga_entry(' ', vga_color);
    }
    if (cursor_row > 0) cursor_row--;
    vga_set_cursor(cursor_row, cursor_col);
}

void vga_putchar(char c) {
    if (c == '\n') {
        cursor_col = 0;
        cursor_row++;
    } else if (c == '\r') {
        cursor_col = 0;
    } else if (c == '\b') {
        if (cursor_col > 0) {
            cursor_col--;
            vga_buffer[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(' ', vga_color);
            vga_set_cursor(cursor_row, cursor_col);
        }
        return;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 8) & ~7;
    } else {
        vga_buffer[cursor_row * VGA_WIDTH + cursor_col] = vga_entry(c, vga_color);
        cursor_col++;
    }

    if (cursor_col >= VGA_WIDTH) {
        cursor_col = 0;
        cursor_row++;
    }
    if (cursor_row >= VGA_HEIGHT) {
        vga_scroll();
    }
    vga_set_cursor(cursor_row, cursor_col);
}

void vga_puts(const char* str) {
    while (*str) vga_putchar(*str++);
}

void vga_puthex(uint32_t n) {
    char buf[11];
    itoa((int)n, buf, 16);
    vga_puts("0x");
    vga_puts(buf);
}

/* Mode 13h graphics (320x200, 256 colors) */
static uint8_t* mode13_buffer = (uint8_t*)0xA0000;
static bool mode13_active = false;

void vga_mode13_init(void) {
    /* Disable interrupts briefly during mode switch */
    __asm__ volatile ("cli");

    outb(0x3C2, 0x63);
    outb(0x3D4, 0x11); outb(0x3D5, (uint8_t)(inb(0x3D5) & 0x7F));

    outb(0x3C0, 0x00); inb(0x3DA);
    outb(0x3C0, 0x01); inb(0x3DA);
    outb(0x3C0, 0x02); inb(0x3DA);
    outb(0x3C0, 0x03); inb(0x3DA);
    outb(0x3C0, 0x04); inb(0x3DA);
    outb(0x3C0, 0x05); inb(0x3DA);
    outb(0x3C0, 0x06); inb(0x3DA);
    outb(0x3C0, 0x07); inb(0x3DA);
    outb(0x3C0, 0x08); inb(0x3DA);
    outb(0x3C0, 0x09); inb(0x3DA);
    outb(0x3C0, 0x0A); inb(0x3DA);
    outb(0x3C0, 0x0B); inb(0x3DA);
    outb(0x3C0, 0x0C); inb(0x3DA);
    outb(0x3C0, 0x0D); inb(0x3DA);
    outb(0x3C0, 0x0E); inb(0x3DA);
    outb(0x3C0, 0x0F); inb(0x3DA);

    outb(0x3D4, 0x00); outb(0x3D5, 0x5F);
    outb(0x3D4, 0x01); outb(0x3D5, 0x4F);
    outb(0x3D4, 0x02); outb(0x3D5, 0x50);
    outb(0x3D4, 0x03); outb(0x3D5, 0x82);
    outb(0x3D4, 0x04); outb(0x3D5, 0x54);
    outb(0x3D4, 0x05); outb(0x3D5, 0x80);
    outb(0x3D4, 0x06); outb(0x3D5, 0x0B);
    outb(0x3D4, 0x07); outb(0x3D5, 0x3E);
    outb(0x3D4, 0x08); outb(0x3D5, 0x00);
    outb(0x3D4, 0x09); outb(0x3D5, 0x40);
    outb(0x3D4, 0x10); outb(0x3D5, 0x9C);
    outb(0x3D4, 0x11); outb(0x3D5, 0x0E);
    outb(0x3D4, 0x12); outb(0x3D5, 0x8F);
    outb(0x3D4, 0x13); outb(0x3D5, 0x28);
    outb(0x3D4, 0x14); outb(0x3D5, 0x40);
    outb(0x3D4, 0x15); outb(0x3D5, 0x96);
    outb(0x3D4, 0x16); outb(0x3D5, 0xB9);
    outb(0x3D4, 0x17); outb(0x3D5, 0xA3);

    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x01);
    outb(0x3C4, 0x02); outb(0x3C5, 0x08);
    outb(0x3C4, 0x04); outb(0x3C5, 0x06);

    outb(0x3CE, 0x00); outb(0x3CF, 0x00);
    outb(0x3CE, 0x01); outb(0x3CF, 0x40);
    outb(0x3CE, 0x02); outb(0x3CF, 0x05);
    outb(0x3CE, 0x03); outb(0x3CF, 0x0E);
    outb(0x3CE, 0x04); outb(0x3CF, 0x00);

    outb(0x3C0, 0x20);
    inb(0x3DA);

    mode13_active = true;
    memset(mode13_buffer, 0, 320 * 200);

    __asm__ volatile ("sti");
}

void vga_text_restore(void) {
    __asm__ volatile ("cli");

    outb(0x3C2, 0x67);
    outb(0x3D4, 0x11); outb(0x3D5, (uint8_t)(inb(0x3D5) & 0x7F));

    outb(0x3C4, 0x00); outb(0x3C5, 0x03);
    outb(0x3C4, 0x01); outb(0x3C5, 0x00);
    outb(0x3C4, 0x02); outb(0x3C5, 0x03);
    outb(0x3C4, 0x04); outb(0x3C5, 0x02);

    outb(0x3D4, 0x00); outb(0x3D5, 0x5F);
    outb(0x3D4, 0x01); outb(0x3D5, 0x4F);
    outb(0x3D4, 0x02); outb(0x3D5, 0x50);
    outb(0x3D4, 0x03); outb(0x3D5, 0x82);
    outb(0x3D4, 0x04); outb(0x3D5, 0x55);
    outb(0x3D4, 0x05); outb(0x3D5, 0x81);
    outb(0x3D4, 0x06); outb(0x3D5, 0xBF);
    outb(0x3D4, 0x07); outb(0x3D5, 0x1F);
    outb(0x3D4, 0x08); outb(0x3D5, 0x00);
    outb(0x3D4, 0x09); outb(0x3D5, 0x4F);
    outb(0x3D4, 0x10); outb(0x3D5, 0x9C);
    outb(0x3D4, 0x11); outb(0x3D5, 0x0E);
    outb(0x3D4, 0x12); outb(0x3D5, 0x8F);
    outb(0x3D4, 0x13); outb(0x3D5, 0x28);
    outb(0x3D4, 0x14); outb(0x3D5, 0x1F);
    outb(0x3D4, 0x15); outb(0x3D5, 0x96);
    outb(0x3D4, 0x16); outb(0x3D5, 0xB9);
    outb(0x3D4, 0x17); outb(0x3D5, 0xA3);

    outb(0x3C0, 0x20);
    inb(0x3DA);

    mode13_active = false;

    __asm__ volatile ("sti");
    vga_init();
}

void vga_mode13_clear(uint8_t color) {
    if (!mode13_active) return;
    memset(mode13_buffer, color, 320 * 200);
}

void vga_mode13_putpixel(int x, int y, uint8_t color) {
    if (!mode13_active || x < 0 || x >= 320 || y < 0 || y >= 200) return;
    mode13_buffer[y * 320 + x] = color;
}

void vga_mode13_fill_rect(int x, int y, int w, int h, uint8_t color) {
    if (!mode13_active) return;
    if (x < 0) { w += x; x = 0; }
    if (y < 0) { h += y; y = 0; }
    if (x + w > 320) w = 320 - x;
    if (y + h > 200) h = 200 - y;
    if (w <= 0 || h <= 0) return;

    for (int j = 0; j < h; j++) {
        uint8_t* row = mode13_buffer + (y + j) * 320 + x;
        for (int i = 0; i < w; i++)
            row[i] = color;
    }
}

void vga_mode13_draw_rect(int x, int y, int w, int h, uint8_t color) {
    for (int i = x; i < x + w; i++) {
        vga_mode13_putpixel(i, y, color);
        vga_mode13_putpixel(i, y + h - 1, color);
    }
    for (int j = y; j < y + h; j++) {
        vga_mode13_putpixel(x, j, color);
        vga_mode13_putpixel(x + w - 1, j, color);
    }
}
