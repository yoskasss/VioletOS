/* VioletOS - Basic web browser (mouse enabled only here) */
#include "violet.h"

#define BROWSER_CONTENT_MAX 3500
#define BROWSER_URL_MAX     64

static char browser_url[BROWSER_URL_MAX] = "example.com";
static char browser_path[64] = "/";
static char browser_page[BROWSER_CONTENT_MAX];
static int  browser_scroll = 0;
static bool browser_editing_url = false;

static void browser_draw_text(int x, int y, const char* s, uint8_t fg, uint8_t bg) {
    int cx = x;
    while (*s && cx < 310) {
        for (int dy = 0; dy < 8; dy++)
            for (int dx = 0; dx < 6; dx++)
                vga_mode13_putpixel(cx + dx, y + dy, bg);
        if (*s != ' ') {
            for (int dy = 0; dy < 8; dy++)
                for (int dx = 0; dx < 5; dx++)
                    if ((dx + dy + (*s)) % 3 == 0)
                        vga_mode13_putpixel(cx + dx, y + dy, fg);
        }
        cx += 6;
        s++;
    }
}

static void browser_draw_cursor(int mx, int my) {
    for (int i = 0; i < 10; i++) {
        vga_mode13_putpixel(mx + i, my + 5, 15);
        vga_mode13_putpixel(mx + 5, my + i, 15);
    }
}

static void browser_render(void) {
    vga_mode13_fill_rect(0, 0, 320, 200, 1);
    vga_mode13_fill_rect(0, 0, 320, 14, 3);
    browser_draw_text(4, 3, "Violet Browser", 15, 3);
    browser_draw_text(4, 18, "URL:", 15, 1);
    browser_draw_text(28, 18, browser_url, 15, browser_editing_url ? 4 : 1);
    browser_draw_text(28 + (int)strlen(browser_url) * 6, 18, browser_path, 14, 1);
    browser_draw_text(268, 18, "[Go]", 15, 2);

    vga_mode13_fill_rect(0, 30, 320, 170, 0);
    int y = 32 - browser_scroll;
    const char* p = browser_page;
    char line[48];
    int li = 0;

    while (*p && y < 200) {
        while (*p && *p != '\n' && li < 47) line[li++] = *p++;
        line[li] = '\0';
        if (y >= 30)
            browser_draw_text(4, y, line, 15, 0);
        li = 0;
        y += 10;
        if (*p == '\n') p++;
    }

    if (!browser_page[0])
        browser_draw_text(4, 40, "Press G or click Go. Default: example.com", 7, 0);

    int mx = 160, my = 100;
    browser_draw_cursor(mx, my);
}

static void browser_fetch(void) {
    browser_draw_text(4, 40, "Offline mode", 14, 0);
    strcpy(browser_page, "Internet access is disabled.\nThis browser only shows the local UI.");
    browser_scroll = 0;
}

void browser_run(void) {
    keyboard_flush();
    keyboard_wait_release();

    vga_mode13_init();
    vga_mode13_clear(1);

    strcpy(browser_page, "");
    browser_scroll = 0;
    browser_editing_url = false;
    browser_fetch();

    bool running = true;
    bool prev_left = false;

    while (running) {
        browser_render();

        if (keyboard_poll(NULL)) {
            char c = keyboard_getchar();
            if (c == 27) {
                running = false;
                break;
            }
            if (c == 'g' || c == 'G') {
                browser_fetch();
                continue;
            }
            if (browser_editing_url) {
                if (c == KEY_ENTER || c == '\r') {
                    browser_editing_url = false;
                    browser_fetch();
                } else if (c == KEY_BS) {
                    int len = (int)strlen(browser_url);
                    if (len > 0) browser_url[len - 1] = '\0';
                } else if ((unsigned char)c >= 32 && strlen(browser_url) < BROWSER_URL_MAX - 2) {
                    char t[2] = { c, 0 };
                    strcat(browser_url, t);
                }
            } else {
                if (c == (char)KEY_UP) browser_scroll -= 10;
                if (c == (char)KEY_DOWN) browser_scroll += 10;
                if (c == 'u' || c == 'U') browser_editing_url = true;
                if (browser_scroll < 0) browser_scroll = 0;
            }
        }

        __asm__ volatile ("sti; hlt");
    }

    keyboard_flush();
    vga_text_restore();
    vga_clear();
    vga_set_color(15, 0);
}
