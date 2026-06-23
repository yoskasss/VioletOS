/* VioletOS - Text menus (keyboard only) */
#include "violet.h"

#define COL_FG  13
#define COL_BG  0
#define COL_HI  15

static const char* logo[] = {
    " __     ___ _       _      _  ____   ___  ",
    " \\ \\   / (_) |_ ___| | ___| |/ ___| / _ \\ ",
    "  \\ \\ / /| | __/ _ \\ |/ _ \\ | |  _ | | | |",
    "   \\ V / | | ||  __/ |  __/ | |_| || |_| |",
    "    \\_/  |_|\\__\\___|_|\\___|_|\\____| \\___/ ",
    NULL
};

static void draw_box(int row, int col, int width, const char* title) {
    vga_set_color(COL_FG, COL_BG);
    vga_set_cursor(row, col);
    for (int i = 0; i < width; i++) vga_putchar('=');
    vga_set_cursor(row + 1, col + (width - (int)strlen(title)) / 2);
    vga_puts(title);
    vga_set_cursor(row + 2, col);
    for (int i = 0; i < width; i++) vga_putchar('=');
}

static int menu_select(const char* title, const char** items, int count, int start_row) {
    int selected = 0;
    int col = 28;
    int width = 24;

    while (1) {
        vga_clear();
        vga_set_color(COL_FG, COL_BG);
        draw_box(start_row, col, width, title);

        for (int i = 0; i < count; i++) {
            vga_set_cursor(start_row + 4 + i * 2, col + 2);
            if (i == selected) {
                vga_set_color(COL_HI, COL_FG);
                vga_puts("> ");
                vga_puts(items[i]);
                vga_set_color(COL_FG, COL_BG);
            } else {
                vga_puts("  ");
                vga_puts(items[i]);
            }
        }

        vga_set_cursor(start_row + 4 + count * 2 + 1, col + 2);
        vga_set_color(COL_FG, COL_BG);
        vga_puts("Arrow Keys + Enter");

        char key = keyboard_getchar();
        if (key == (char)KEY_UP || key == 'w' || key == 'W')
            selected = (selected - 1 + count) % count;
        else if (key == (char)KEY_DOWN || key == 's' || key == 'S')
            selected = (selected + 1) % count;
        else if (key == KEY_ENTER || key == '\r')
            return selected;
        else if (key >= '1' && key <= '9' && (key - '1') < count)
            return key - '1';
    }
}

void gui_show_logo(void) {
    vga_set_color(COL_FG, COL_BG);
    vga_clear();
    for (int i = 0; logo[i]; i++) {
        vga_set_cursor(3 + i, 20);
        vga_puts(logo[i]);
    }
    vga_set_cursor(10, 26);
    vga_set_color(COL_HI, COL_BG);
    vga_puts("VioletOS v");
    vga_puts(VIOLETOS_VERSION);
    vga_set_cursor(22, 30);
    vga_set_color(COL_FG, COL_BG);
    vga_puts("Booting...");
    delay_ms(1200);
}

void gui_main_menu(void) {
    const char* items[] = { "Terminal", "Violet IDE", "Shutdown" };

    keyboard_flush();
    while (1) {
        vga_set_window_title("VioletOS - Ana Menu");
        int choice = menu_select("VIOLETOS", items, 3, 5);
        if (choice == 0) {
            vga_clear();
            vga_set_color(COL_HI, COL_BG);
            shell_run();
            keyboard_flush();
        } else if (choice == 1) {
            keyboard_wait_release();
            vga_clear();
            ide_run();
            keyboard_flush();
        } else if (choice == 2) {
            sys_shutdown();
        }
    }
}

