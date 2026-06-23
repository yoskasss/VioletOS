/* VioletOS - VBE 800x600x32bpp graphics mode and virtual window driver */
#include "violet.h"
#include "port.h"

#define VGA_WIDTH  80
#define VGA_HEIGHT 25

/* Standard VGA fallback */
#define VGA_MEMORY 0xB8000
static uint16_t* vga_text_buffer = (uint16_t*)VGA_MEMORY;

/* VBE Graphics Framebuffer fields */
static uint32_t* lfb_vram = NULL;
static bool graphics_active = false;
static char current_window_title[64] = "VioletOS";
static uint16_t virtual_screen[VGA_WIDTH * VGA_HEIGHT];
static int cursor_row = 0;
static int cursor_col = 0;
static uint8_t vga_color_attr = 0x0F;
static bool hw_cursor_visible = false;

/* Double buffer for drawing to prevent flickering */
static uint32_t lfb_backbuffer[800 * 600];

/* 8x8 bitmap font (covers ASCII 32 - 127) */
static const uint8_t font8x8[96][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // space
    {0x18, 0x3c, 0x3c, 0x18, 0x18, 0x00, 0x18, 0x00}, // !
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // "
    {0x36, 0x36, 0x7f, 0x36, 0x7f, 0x36, 0x36, 0x00}, // #
    {0x0c, 0x3e, 0x03, 0x1e, 0x30, 0x1f, 0x0c, 0x00}, // $
    {0x00, 0x63, 0x33, 0x18, 0x0c, 0x66, 0x63, 0x00}, // %
    {0x1c, 0x36, 0x1c, 0x2e, 0x66, 0x3b, 0x1d, 0x00}, // &
    {0x18, 0x18, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00}, // '
    {0x0c, 0x18, 0x30, 0x30, 0x30, 0x18, 0x0c, 0x00}, // (
    {0x30, 0x18, 0x0c, 0x0c, 0x0c, 0x18, 0x30, 0x00}, // )
    {0x00, 0x66, 0x3c, 0xff, 0x3c, 0x66, 0x00, 0x00}, // *
    {0x00, 0x18, 0x18, 0x7e, 0x18, 0x18, 0x00, 0x00}, // +
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x0c}, // ,
    {0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00}, // -
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x18, 0x18, 0x00}, // .
    {0x00, 0x03, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x00}, // /
    {0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00}, // 0
    {0x0c, 0x0e, 0x0c, 0x0c, 0x0c, 0x0c, 0x3f, 0x00}, // 1
    {0x3e, 0x63, 0x03, 0x0e, 0x30, 0x60, 0x7f, 0x00}, // 2
    {0x3e, 0x63, 0x03, 0x1e, 0x03, 0x63, 0x3e, 0x00}, // 3
    {0x06, 0x0e, 0x1e, 0x36, 0x7f, 0x06, 0x06, 0x00}, // 4
    {0x7f, 0x60, 0x7e, 0x03, 0x03, 0x63, 0x3e, 0x00}, // 5
    {0x1c, 0x30, 0x60, 0x7e, 0x63, 0x63, 0x3e, 0x00}, // 6
    {0x7f, 0x03, 0x06, 0x0c, 0x18, 0x18, 0x18, 0x00}, // 7
    {0x3e, 0x63, 0x63, 0x3e, 0x63, 0x63, 0x3e, 0x00}, // 8
    {0x3e, 0x63, 0x63, 0x7f, 0x03, 0x06, 0x3c, 0x00}, // 9
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x00, 0x00}, // :
    {0x00, 0x18, 0x18, 0x00, 0x18, 0x18, 0x0c, 0x00}, // ;
    {0x0e, 0x18, 0x30, 0x60, 0x30, 0x18, 0x0e, 0x00}, // <
    {0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00}, // =
    {0x70, 0x18, 0x0c, 0x06, 0x0c, 0x18, 0x70, 0x00}, // >
    {0x3e, 0x63, 0x06, 0x0c, 0x18, 0x00, 0x18, 0x00}, // ?
    {0x3e, 0x63, 0x6f, 0x6b, 0x6b, 0x60, 0x3e, 0x00}, // @
    {0x18, 0x3c, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x00}, // A
    {0x7e, 0x33, 0x33, 0x3e, 0x33, 0x33, 0x7e, 0x00}, // B
    {0x1e, 0x33, 0x60, 0x60, 0x60, 0x33, 0x1e, 0x00}, // C
    {0x7c, 0x36, 0x33, 0x33, 0x33, 0x36, 0x7c, 0x00}, // D
    {0x7f, 0x60, 0x60, 0x78, 0x60, 0x60, 0x7f, 0x00}, // E
    {0x7f, 0x60, 0x60, 0x78, 0x60, 0x60, 0x60, 0x00}, // F
    {0x3e, 0x63, 0x60, 0x67, 0x63, 0x63, 0x3e, 0x00}, // G
    {0x66, 0x66, 0x66, 0x7e, 0x66, 0x66, 0x66, 0x00}, // H
    {0x3c, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // I
    {0x0f, 0x06, 0x06, 0x06, 0x06, 0x66, 0x3c, 0x00}, // J
    {0x66, 0x6c, 0x78, 0x70, 0x78, 0x6c, 0x66, 0x00}, // K
    {0x60, 0x60, 0x60, 0x60, 0x60, 0x60, 0x7f, 0x00}, // L
    {0x63, 0x77, 0x7f, 0x6b, 0x63, 0x63, 0x63, 0x00}, // M
    {0x66, 0x76, 0x7e, 0x7e, 0x6e, 0x66, 0x66, 0x00}, // N
    {0x3e, 0x63, 0x63, 0x63, 0x63, 0x63, 0x3e, 0x00}, // O
    {0x7e, 0x63, 0x63, 0x7e, 0x60, 0x60, 0x60, 0x00}, // P
    {0x3e, 0x63, 0x63, 0x63, 0x6b, 0x66, 0x3d, 0x00}, // Q
    {0x7e, 0x63, 0x63, 0x7e, 0x70, 0x6c, 0x66, 0x00}, // R
    {0x3e, 0x63, 0x38, 0x0e, 0x03, 0x63, 0x3e, 0x00}, // S
    {0x7e, 0x18, 0x18, 0x18, 0x18, 0x18, 0x18, 0x00}, // T
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x00}, // U
    {0x66, 0x66, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // V
    {0x63, 0x63, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // W
    {0x66, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x66, 0x00}, // X
    {0x66, 0x66, 0x66, 0x3c, 0x18, 0x18, 0x18, 0x00}, // Y
    {0x7f, 0x06, 0x0c, 0x18, 0x30, 0x60, 0x7f, 0x00}, // Z
    {0x3c, 0x30, 0x30, 0x30, 0x30, 0x30, 0x3c, 0x00}, // [
    {0x00, 0x60, 0x30, 0x18, 0x0c, 0x06, 0x03, 0x00}, // backslash
    {0x3c, 0x0c, 0x0c, 0x0c, 0x0c, 0x0c, 0x3c, 0x00}, // ]
    {0x08, 0x1c, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // ^
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xff}, // _
    {0x0c, 0x0c, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // `
    {0x00, 0x00, 0x3c, 0x06, 0x3e, 0x66, 0x3b, 0x00}, // a
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x7c, 0x00}, // b
    {0x00, 0x00, 0x3c, 0x60, 0x60, 0x60, 0x3c, 0x00}, // c
    {0x06, 0x06, 0x3e, 0x66, 0x66, 0x66, 0x3e, 0x00}, // d
    {0x00, 0x00, 0x3c, 0x66, 0x7e, 0x60, 0x3c, 0x00}, // e
    {0x1c, 0x30, 0x78, 0x30, 0x30, 0x30, 0x30, 0x00}, // f
    {0x00, 0x00, 0x3b, 0x66, 0x66, 0x3e, 0x06, 0x3c}, // g
    {0x60, 0x60, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // h
    {0x18, 0x00, 0x38, 0x18, 0x18, 0x18, 0x3c, 0x00}, // i
    {0x06, 0x00, 0x0e, 0x06, 0x06, 0x06, 0x06, 0x3c}, // j
    {0x60, 0x60, 0x66, 0x6c, 0x78, 0x6c, 0x66, 0x00}, // k
    {0x38, 0x18, 0x18, 0x18, 0x18, 0x18, 0x3c, 0x00}, // l
    {0x00, 0x00, 0x66, 0x7f, 0x7f, 0x6b, 0x63, 0x00}, // m
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x66, 0x66, 0x00}, // n
    {0x00, 0x00, 0x3c, 0x66, 0x66, 0x66, 0x3c, 0x00}, // o
    {0x00, 0x00, 0x7c, 0x66, 0x66, 0x7c, 0x60, 0x60}, // p
    {0x00, 0x00, 0x3e, 0x66, 0x66, 0x3e, 0x06, 0x07}, // q
    {0x00, 0x00, 0x7c, 0x66, 0x60, 0x60, 0x60, 0x00}, // r
    {0x00, 0x00, 0x3e, 0x60, 0x3c, 0x06, 0x7c, 0x00}, // s
    {0x18, 0x7c, 0x18, 0x18, 0x18, 0x18, 0x0c, 0x00}, // t
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x66, 0x3b, 0x00}, // u
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3c, 0x18, 0x00}, // v
    {0x00, 0x00, 0x63, 0x6b, 0x7f, 0x77, 0x63, 0x00}, // w
    {0x00, 0x00, 0x66, 0x3c, 0x18, 0x3c, 0x66, 0x00}, // x
    {0x00, 0x00, 0x66, 0x66, 0x66, 0x3e, 0x06, 0x3c}, // y
    {0x00, 0x00, 0x7f, 0x0c, 0x18, 0x30, 0x7f, 0x00}, // z
    {0x0c, 0x18, 0x18, 0x70, 0x18, 0x18, 0x0c, 0x00}, // {
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // |
    {0x30, 0x18, 0x18, 0x0e, 0x18, 0x18, 0x30, 0x00}, // }
    {0x00, 0x00, 0x38, 0x6c, 0x38, 0x00, 0x00, 0x00}, // ~
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}  // block cursor
};

/* Color Palette mapping to RGB values */
static uint32_t vga_to_rgb(uint8_t color_index) {
    switch (color_index & 0x0F) {
        case 0:  return 0x000000; // Black
        case 1:  return 0x0B0066; // Blue
        case 2:  return 0x008E2D; // Green
        case 3:  return 0x008E8E; // Cyan
        case 4:  return 0x8E0000; // Red
        case 5:  return 0x8B2DF5; // Purple/Magenta (Accent)
        case 6:  return 0x8E4700; // Brown
        case 7:  return 0xAAAAAA; // Light Grey
        case 8:  return 0x555555; // Dark Grey
        case 9:  return 0x3F3FFF; // Light Blue
        case 10: return 0x3FFF3F; // Light Green
        case 11: return 0x3FFFFF; // Light Cyan
        case 12: return 0xFF3F3F; // Light Red
        case 13: return 0xC394FF; // Light Purple
        case 14: return 0xFFFF3F; // Yellow
        case 15: return 0xFFFFFF; // White
        default: return 0x000000;
    }
}

/* Helper function to draw a solid pixel to backbuffer */
static inline void putpixel(int x, int y, uint32_t color) {
    if (x >= 0 && x < 800 && y >= 0 && y < 600) {
        lfb_backbuffer[y * 800 + x] = color;
    }
}

/* Helper function to fill a rectangle in backbuffer */
static void fill_rect(int x, int y, int w, int h, uint32_t color) {
    for (int cy = y; cy < y + h; cy++) {
        for (int cx = x; cx < x + w; cx++) {
            putpixel(cx, cy, color);
        }
    }
}

/* Helper to draw a single character scaled 1x2 to yield 8x16 fonts */
static void draw_char(int x, int y, char c, uint32_t fg, uint32_t bg, bool transparent) {
    if ((unsigned char)c < 32 || (unsigned char)c > 127) {
        c = 127; // Draw block
    }
    int font_idx = c - 32;
    for (int dy = 0; dy < 16; dy++) {
        uint8_t row_bits = font8x8[font_idx][dy / 2];
        for (int dx = 0; dx < 8; dx++) {
            if (row_bits & (1 << (7 - dx))) {
                putpixel(x + dx, y + dy, fg);
            } else if (!transparent) {
                putpixel(x + dx, y + dy, bg);
            }
        }
    }
}

/* Helper to draw a string */
static void draw_string(int x, int y, const char* str, uint32_t fg, uint32_t bg, bool transparent) {
    while (*str) {
        draw_char(x, y, *str++, fg, bg, transparent);
        x += 8;
    }
}

/* Invert colors of a rectangular region (used for cursor) */
static void invert_rect(int x, int y, int w, int h) {
    for (int cy = y; cy < y + h; cy++) {
        for (int cx = x; cx < x + w; cx++) {
            if (cx >= 0 && cx < 800 && cy >= 0 && cy < 600) {
                lfb_backbuffer[cy * 800 + cx] ^= 0xFFFFFF;
            }
        }
    }
}

/* Screen Refresher (Window Manager & Rendering Logic) */
static void vga_refresh_screen(void) {
    if (!graphics_active || !lfb_vram) {
        return;
    }

    /* 1. Draw beautiful dark purple desktop wallpaper gradient */
    for (int y = 0; y < 600; y++) {
        uint8_t r = 18 + (y * 22) / 600;
        uint8_t g = 6 + (y * 12) / 600;
        uint8_t b = 38 + (y * 42) / 600;
        uint32_t color = (r << 16) | (g << 8) | b;
        for (int x = 0; x < 800; x++) {
            lfb_backbuffer[y * 800 + x] = color;
        }
    }

    /* 2. Draw modern Taskbar at the bottom (height 40px) */
    fill_rect(0, 560, 800, 40, 0x0F071B); // dark purple bar
    fill_rect(0, 560, 800, 2, 0x8B2DF5);   // accent top line
    draw_string(16, 572, "VioletOS v1.0.0", 0xC394FF, 0, true);
    draw_string(220, 572, ":D", 0xAAAAAA, 0, true);

    /* 3. Floating window configuration & drawing */
    /* Dimensions: 640 width (80 chars * 8px), 400 height (25 chars * 16px) */
    int win_w = 640;
    int win_h = 400;
    int titlebar_h = 24;
    int border_size = 4;

    /* Centered positions */
    int wx = (800 - (win_w + border_size * 2)) / 2;
    int wy = (600 - (win_h + titlebar_h + border_size)) / 2;

    /* 3.1. Draw Window shadow (bottom-right offset) */
    fill_rect(wx + 8, wy + 8, win_w + border_size * 2, win_h + titlebar_h + border_size, 0x0A0314);

    /* 3.2. Draw Window border frames */
    fill_rect(wx, wy, win_w + border_size * 2, win_h + titlebar_h + border_size, 0xC394FF); // border outline
    fill_rect(wx + border_size, wy + titlebar_h, win_w, win_h, 0x05010B); // content body container (dark purple-black)

    /* 3.3. Draw Active Titlebar */
    fill_rect(wx + border_size, wy + border_size, win_w, titlebar_h - border_size, 0x8B2DF5); // active purple titlebar
    
    /* Draw Window control buttons (minimize, maximize, close dots on left) */
    fill_rect(wx + 12, wy + 8, 8, 8, 0xFF3F3F); // red close
    fill_rect(wx + 26, wy + 8, 8, 8, 0xFFFF3F); // yellow min
    fill_rect(wx + 40, wy + 8, 8, 8, 0x3FFF3F); // green max

    /* Draw window title centered */
    int title_len = 0;
    while (current_window_title[title_len]) title_len++;
    int title_x = wx + border_size + (win_w - title_len * 8) / 2;
    draw_string(title_x, wy + 6, current_window_title, 0xFFFFFF, 0, true);

    /* 3.4. Draw Console content from virtual screen buffer */
    for (int row = 0; row < VGA_HEIGHT; row++) {
        for (int col = 0; col < VGA_WIDTH; col++) {
            uint16_t cell = virtual_screen[row * VGA_WIDTH + col];
            char c = cell & 0xFF;
            uint8_t attr = (cell >> 8) & 0xFF;
            uint32_t fg = vga_to_rgb(attr & 0x0F);
            uint32_t bg = vga_to_rgb((attr >> 4) & 0x0F);

            int cx = wx + border_size + col * 8;
            int cy = wy + titlebar_h + row * 16;
            
            // Draw character
            draw_char(cx, cy, c, fg, bg, false);
        }
    }

    /* 3.5. Draw Cursor */
    if (hw_cursor_visible) {
        int cur_x = wx + border_size + cursor_col * 8;
        int cur_y = wy + titlebar_h + cursor_row * 16;
        invert_rect(cur_x, cur_y, 8, 16);
    }

    /* 4. Blit backbuffer to VRAM */
    memcpy(lfb_vram, lfb_backbuffer, 800 * 600 * 4);
}

/* public api */

void vga_set_window_title(const char* title) {
    int i = 0;
    while (title[i] && i < 63) {
        current_window_title[i] = title[i];
        i++;
    }
    current_window_title[i] = '\0';
    vga_refresh_screen();
}

void vga_init(struct multiboot_info* mb_info) {
    /* Check if multiboot provided VBE linear framebuffer */
    if (mb_info && (mb_info->flags & 0x1000) && mb_info->framebuffer_addr) {
        lfb_vram = (uint32_t*)(uint32_t)mb_info->framebuffer_addr;
        graphics_active = true;
    } else {
        lfb_vram = NULL;
        graphics_active = false;
    }

    vga_set_color(13, 0); // Light purple default
    vga_hide_hw_cursor();
    vga_clear();
}

void vga_set_color(uint8_t fg, uint8_t bg) {
    vga_color_attr = fg | (bg << 4);
}

void vga_clear(void) {
    for (int i = 0; i < VGA_WIDTH * VGA_HEIGHT; i++) {
        virtual_screen[i] = ' ' | ((uint16_t)vga_color_attr << 8);
    }
    cursor_row = 0;
    cursor_col = 0;
    vga_set_cursor(0, 0);
    vga_refresh_screen();
}

void vga_set_cursor(int row, int col) {
    if (row < 0) row = 0;
    if (col < 0) col = 0;
    if (row >= VGA_HEIGHT) row = VGA_HEIGHT - 1;
    if (col >= VGA_WIDTH) col = VGA_WIDTH - 1;
    cursor_row = row;
    cursor_col = col;

    if (!graphics_active) {
        /* Standard VGA hardware cursor position register updates */
        uint16_t pos = (uint16_t)(row * VGA_WIDTH + col);
        outb(0x3D4, 0x0F);
        outb(0x3D5, (uint8_t)(pos & 0xFF));
        outb(0x3D4, 0x0E);
        outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
    } else {
        vga_refresh_screen();
    }
}

void vga_get_cursor(int* row, int* col) {
    if (row) *row = cursor_row;
    if (col) *col = cursor_col;
}

int vga_get_rows(void) { return VGA_HEIGHT; }
int vga_get_cols(void) { return VGA_WIDTH; }

void vga_putchar_at(int row, int col, char c, uint8_t fg, uint8_t bg) {
    if (row < 0 || col < 0 || row >= VGA_HEIGHT || col >= VGA_WIDTH) return;
    virtual_screen[row * VGA_WIDTH + col] = (unsigned char)c | ((uint16_t)(fg | (bg << 4)) << 8);
    
    if (!graphics_active) {
        vga_text_buffer[row * VGA_WIDTH + col] = (unsigned char)c | ((uint16_t)(fg | (bg << 4)) << 8);
    }
    /* We don't trigger refresh here as it might be called in loops. We trigger refresh on screen updates/cursor moves. */
}

uint16_t vga_get_cell(int row, int col) {
    if (row < 0 || col < 0 || row >= VGA_HEIGHT || col >= VGA_WIDTH) {
        return ' ' | ((uint16_t)vga_color_attr << 8);
    }
    return virtual_screen[row * VGA_WIDTH + col];
}

void vga_hide_hw_cursor(void) {
    hw_cursor_visible = false;
    if (!graphics_active) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x20);
    } else {
        vga_refresh_screen();
    }
}

void vga_show_hw_cursor(void) {
    hw_cursor_visible = true;
    if (!graphics_active) {
        outb(0x3D4, 0x0A);
        outb(0x3D5, 0x0E);
        outb(0x3D4, 0x0B);
        outb(0x3D5, 0x0F);
    } else {
        vga_refresh_screen();
    }
}

void vga_scroll(void) {
    for (int y = 0; y < VGA_HEIGHT - 1; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            virtual_screen[y * VGA_WIDTH + x] = virtual_screen[(y + 1) * VGA_WIDTH + x];
            if (!graphics_active) {
                vga_text_buffer[y * VGA_WIDTH + x] = vga_text_buffer[(y + 1) * VGA_WIDTH + x];
            }
        }
    }
    for (int x = 0; x < VGA_WIDTH; x++) {
        virtual_screen[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ' ' | ((uint16_t)vga_color_attr << 8);
        if (!graphics_active) {
            vga_text_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] = ' ' | ((uint16_t)vga_color_attr << 8);
        }
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
            virtual_screen[cursor_row * VGA_WIDTH + cursor_col] = ' ' | ((uint16_t)vga_color_attr << 8);
            if (!graphics_active) {
                vga_text_buffer[cursor_row * VGA_WIDTH + cursor_col] = ' ' | ((uint16_t)vga_color_attr << 8);
            }
            vga_set_cursor(cursor_row, cursor_col);
        }
        return;
    } else if (c == '\t') {
        cursor_col = (cursor_col + 8) & ~7;
    } else {
        virtual_screen[cursor_row * VGA_WIDTH + cursor_col] = (unsigned char)c | ((uint16_t)vga_color_attr << 8);
        if (!graphics_active) {
            vga_text_buffer[cursor_row * VGA_WIDTH + cursor_col] = (unsigned char)c | ((uint16_t)vga_color_attr << 8);
        }
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
    while (*str) {
        vga_putchar(*str++);
    }
    vga_refresh_screen();
}

void vga_puthex(uint32_t n) {
    char buf[11];
    itoa((int)n, buf, 16);
    vga_puts("0x");
    vga_puts(buf);
}
