/* VioletOS - Violet IDE */
#include "violet.h"
#include "violet_lang.h"

#define IDE_MAX_CHARS   4096
#define IDE_MAX_LINES   64
#define IDE_TITLE_ROW   0
#define IDE_HEAD_RULE   1
#define IDE_EDIT_START  2
#define IDE_EDIT_ROWS   18
#define IDE_FOOT_RULE   20
#define IDE_CMD_ROW     21
#define IDE_STATUS_ROW  22
#define IDE_GUTTER_COL  4
#define IDE_CODE_COL    6
#define IDE_CODE_WIDTH  73

static char ide_buffer[IDE_MAX_CHARS];
static int  ide_lines[IDE_MAX_LINES];
static int  ide_line_count = 1;
static int  ide_cur_line = 0;
static int  ide_cur_col = 0;
static int  ide_scroll = 0;
static char ide_filename[FS_MAX_NAME] = "untitled.v";
static char ide_status[80];
static bool ide_dirty = false;

static const char* skip_ws(const char* s) {
    while (*s == ' ') s++;
    return s;
}

static void ide_set_status(const char* msg) {
    strncpy(ide_status, msg, sizeof(ide_status) - 1);
    ide_status[sizeof(ide_status) - 1] = '\0';
}

static void ide_init_buffer(void) {
    memset(ide_buffer, 0, sizeof(ide_buffer));
    memset(ide_lines, 0, sizeof(ide_lines));
    ide_line_count = 1;
    ide_cur_line = 0;
    ide_cur_col = 0;
    ide_scroll = 0;
    ide_lines[0] = 0;
    ide_dirty = false;
}

static int ide_line_start(int line) {
    return ide_lines[line];
}

static int ide_line_len(int line) {
    if (line >= ide_line_count) return 0;
    int start = ide_lines[line];
    int end = (line + 1 < ide_line_count) ? ide_lines[line + 1] - 1 : (int)strlen(ide_buffer);
    return end - start;
}

static void ide_rebuild_lines(void) {
    ide_line_count = 0;
    ide_lines[0] = 0;
    int len = (int)strlen(ide_buffer);
    for (int i = 0; i < len && ide_line_count < IDE_MAX_LINES - 1; i++) {
        if (ide_buffer[i] == '\n') {
            ide_line_count++;
            ide_lines[ide_line_count] = i + 1;
        }
    }
    ide_line_count++;
}

static void ide_ensure_scroll(void) {
    if (ide_cur_line < ide_scroll)
        ide_scroll = ide_cur_line;
    else if (ide_cur_line >= ide_scroll + IDE_EDIT_ROWS)
        ide_scroll = ide_cur_line - IDE_EDIT_ROWS + 1;
}

static void ide_insert_char(char c) {
    int pos = ide_line_start(ide_cur_line) + ide_cur_col;
    int len = (int)strlen(ide_buffer);
    if (len >= IDE_MAX_CHARS - 2) return;

    if (c == '\n') {
        memmove(ide_buffer + pos + 1, ide_buffer + pos, len - pos + 1);
        ide_buffer[pos] = '\n';
        for (int i = ide_line_count; i > ide_cur_line + 1; i--)
            ide_lines[i] = ide_lines[i - 1] + 1;
        ide_lines[ide_cur_line + 1] = pos + 1;
        ide_line_count++;
        ide_cur_line++;
        ide_cur_col = 0;
    } else {
        memmove(ide_buffer + pos + 1, ide_buffer + pos, len - pos + 1);
        ide_buffer[pos] = c;
        for (int i = ide_cur_line + 1; i < ide_line_count; i++)
            ide_lines[i]++;
        ide_cur_col++;
    }
    ide_dirty = true;
}

static void ide_delete_char(void) {
    if (ide_cur_col > 0) {
        int pos = ide_line_start(ide_cur_line) + ide_cur_col - 1;
        int len = (int)strlen(ide_buffer);
        memmove(ide_buffer + pos, ide_buffer + pos + 1, len - pos);
        for (int i = ide_cur_line + 1; i < ide_line_count; i++)
            ide_lines[i]--;
        ide_cur_col--;
    } else if (ide_cur_line > 0) {
        int prev = ide_line_len(ide_cur_line - 1);
        int pos = ide_line_start(ide_cur_line);
        int len = ide_line_len(ide_cur_line);
        memmove(ide_buffer + pos - 1, ide_buffer + pos, len + 1);
        for (int i = ide_cur_line; i < ide_line_count - 1; i++)
            ide_lines[i] = ide_lines[i + 1] - len - 1;
        ide_line_count--;
        ide_cur_line--;
        ide_cur_col = prev;
    }
    ide_dirty = true;
}

static void ide_fill_row(int row, char ch, uint8_t fg, uint8_t bg) {
    for (int x = 0; x < 80; x++)
        vga_putchar_at(row, x, ch, fg, bg);
}

static void ide_print_at(int row, int col, const char* s, uint8_t fg, uint8_t bg) {
    while (*s && col < 80)
        vga_putchar_at(row, col++, *s++, fg, bg);
}

static void ide_draw_line_number(int row, int line_no, bool active) {
    char num[8];
    itoa(line_no + 1, num, 10);
    int len = (int)strlen(num);
    int col = IDE_GUTTER_COL - len;
    uint8_t fg = active ? 15 : 8;
    uint8_t bg = active ? 5 : 0;

    for (int x = 0; x <= IDE_GUTTER_COL; x++)
        vga_putchar_at(row, x, ' ', fg, bg);

    ide_print_at(row, col, num, fg, bg);
    vga_putchar_at(row, IDE_GUTTER_COL + 1, '|', active ? 13 : 8, bg);
}

static void ide_draw_cursor_cell(int row, int col) {
    char ch = ' ';
    int len = ide_line_len(ide_cur_line);
    if (ide_cur_col < len)
        ch = ide_buffer[ide_line_start(ide_cur_line) + ide_cur_col];
    vga_putchar_at(row, col, ch, 0, 15);
}

static void ide_render(void) {
    ide_ensure_scroll();

    /* Full screen paint — overwrite any previous menu/content */
    for (int r = 0; r < 25; r++)
        ide_fill_row(r, ' ', 15, 0);

    /* Title bar */
    ide_fill_row(IDE_TITLE_ROW, ' ', 15, 5);
    ide_print_at(IDE_TITLE_ROW, 1, " Violet IDE ", 15, 5);
    ide_print_at(IDE_TITLE_ROW, 15, "File: ", 7, 5);
    ide_print_at(IDE_TITLE_ROW, 21, ide_filename, 15, 5);
    if (ide_dirty)
        ide_print_at(IDE_TITLE_ROW, 21 + (int)strlen(ide_filename), " *", 14, 5);
    ide_print_at(IDE_TITLE_ROW, 56, "Ctrl+S save  Ctrl+R run  Ctrl+Q quit", 7, 5);

    /* Rules */
    ide_fill_row(IDE_HEAD_RULE, '-', 8, 0);
    ide_fill_row(IDE_FOOT_RULE, '-', 8, 0);

    /* Editor body */
    for (int v = 0; v < IDE_EDIT_ROWS; v++) {
        int row = IDE_EDIT_START + v;
        int line = ide_scroll + v;
        bool active = (line == ide_cur_line);

        if (line < ide_line_count) {
            ide_draw_line_number(row, line, active);
            int start = ide_line_start(line);
            int len = ide_line_len(line);
            uint8_t fg = 15;
            uint8_t bg = active ? 1 : 0;
            for (int x = IDE_CODE_COL; x < 80; x++)
                vga_putchar_at(row, x, ' ', fg, bg);
            for (int j = 0; j < len && j < IDE_CODE_WIDTH; j++)
                vga_putchar_at(row, IDE_CODE_COL + j, ide_buffer[start + j], fg, bg);
        } else {
            ide_draw_line_number(row, line, false);
            vga_putchar_at(row, IDE_GUTTER_COL + 1, '|', 8, 0);
            for (int x = IDE_CODE_COL; x < 80; x++)
                vga_putchar_at(row, x, ' ', 15, 0);
        }
    }

    /* Command line */
    ide_fill_row(IDE_CMD_ROW, ' ', 15, 0);
    ide_print_at(IDE_CMD_ROW, 0, ": ", 13, 0);
    ide_print_at(IDE_CMD_ROW, 2, "open | save | new | run", 8, 0);

    /* Status bar */
    ide_fill_row(IDE_STATUS_ROW, ' ', 8, 0);
    ide_print_at(IDE_STATUS_ROW, 1, ide_status, 10, 0);
    char scroll_info[24];
    scroll_info[0] = '\0';
    if (ide_line_count > IDE_EDIT_ROWS) {
        strcat(scroll_info, "Ln ");
        char b[8];
        itoa(ide_cur_line + 1, b, 10);
        strcat(scroll_info, b);
        strcat(scroll_info, "/");
        itoa(ide_line_count, b, 10);
        strcat(scroll_info, b);
    }
    if (scroll_info[0])
        ide_print_at(IDE_STATUS_ROW, 60, scroll_info, 8, 0);

    /* Software cursor */
    int cr = IDE_EDIT_START + (ide_cur_line - ide_scroll);
    int cc = IDE_CODE_COL + ide_cur_col;
    if (cr >= IDE_EDIT_START && cr < IDE_EDIT_START + IDE_EDIT_ROWS)
        ide_draw_cursor_cell(cr, cc);

    vga_hide_hw_cursor();
}

static void ide_save(void) {
    if (fs_write(ide_filename, ide_buffer, strlen(ide_buffer)) >= 0) {
        ide_dirty = false;
        ide_set_status("Saved.");
    } else {
        ide_set_status("Save failed.");
    }
}

static void ide_new(void) {
    ide_init_buffer();
    strcpy(ide_filename, "untitled.v");
    ide_set_status("New file.");
}

static void ide_open(const char* name) {
    char buf[IDE_MAX_CHARS];
    if (fs_read(name, buf, sizeof(buf)) < 0) {
        ide_set_status("Open failed.");
        return;
    }
    strcpy(ide_buffer, buf);
    strncpy(ide_filename, name, FS_MAX_NAME - 1);
    ide_rebuild_lines();
    ide_cur_line = 0;
    ide_cur_col = 0;
    ide_scroll = 0;
    ide_dirty = false;
    ide_set_status("Opened.");
}

static void ide_run_code(void) {
    vga_clear();
    vga_set_color(15, 0);
    vga_show_hw_cursor();
    vga_puts("--- Running ");
    vga_puts(ide_filename);
    vga_puts(" ---\n");
    violet_run_source(ide_buffer);
    vga_puts("\n--- Press any key ---");
    keyboard_getchar();
    ide_set_status("Run complete.");
}

static void ide_exec_command(char* cmd) {
    while (*cmd == ' ') cmd++;
    if (strcmp(cmd, "save") == 0) ide_save();
    else if (strcmp(cmd, "new") == 0) ide_new();
    else if (strcmp(cmd, "run") == 0) ide_run_code();
    else if (strncmp(cmd, "open ", 5) == 0) ide_open(skip_ws(cmd + 5));
    else if (cmd[0]) ide_set_status("Commands: open save new run");
}

static void ide_read_command(void) {
    char cmd[80];
    int pos = 0;

    ide_fill_row(IDE_CMD_ROW, ' ', 15, 0);
    ide_print_at(IDE_CMD_ROW, 0, ": ", 13, 0);
    vga_show_hw_cursor();

    while (1) {
        vga_set_cursor(IDE_CMD_ROW, 2 + pos);
        char c = keyboard_getchar();
        if (c == KEY_ENTER || c == '\r') {
            cmd[pos] = '\0';
            vga_hide_hw_cursor();
            ide_exec_command(cmd);
            return;
        }
        if (c == KEY_BS && pos > 0) {
            pos--;
            cmd[pos] = '\0';
            vga_putchar_at(IDE_CMD_ROW, 2 + pos, ' ', 15, 0);
            vga_set_cursor(IDE_CMD_ROW, 2 + pos);
        } else if (c == 27) {
            vga_hide_hw_cursor();
            ide_set_status("Command cancelled.");
            return;
        } else if (c >= 32 && pos < 79) {
            cmd[pos++] = c;
            vga_putchar_at(IDE_CMD_ROW, 2 + pos - 1, c, 15, 0);
        }
    }
}

void ide_run(void) {
    vga_set_window_title("VioletOS - Violet IDE");
    vga_clear();
    vga_hide_hw_cursor();
    keyboard_flush();
    ide_init_buffer();
    ide_set_status("Type ':' then command, or start coding below");

    while (1) {
        ide_render();

        char c = keyboard_getchar();

        if (c == 0x11) {
            if (ide_dirty) ide_save();
            break;
        }
        if (c == 0x13) {
            ide_save();
            continue;
        }
        if (c == 0x12) {
            ide_run_code();
            continue;
        }
        if (c == ':') {
            ide_read_command();
            continue;
        }
        if (c == (char)KEY_UP) {
            if (ide_cur_line > 0) ide_cur_line--;
            if (ide_cur_col > ide_line_len(ide_cur_line))
                ide_cur_col = ide_line_len(ide_cur_line);
        } else if (c == (char)KEY_DOWN) {
            if (ide_cur_line < ide_line_count - 1) ide_cur_line++;
            if (ide_cur_col > ide_line_len(ide_cur_line))
                ide_cur_col = ide_line_len(ide_cur_line);
        } else if (c == (char)KEY_LEFT) {
            if (ide_cur_col > 0) ide_cur_col--;
            else if (ide_cur_line > 0) {
                ide_cur_line--;
                ide_cur_col = ide_line_len(ide_cur_line);
            }
        } else if (c == (char)KEY_RIGHT) {
            if (ide_cur_col < ide_line_len(ide_cur_line)) ide_cur_col++;
            else if (ide_cur_line < ide_line_count - 1) {
                ide_cur_line++;
                ide_cur_col = 0;
            }
        } else if (c == KEY_BS || c == 0x7F) {
            ide_delete_char();
        } else if (c == KEY_ENTER || c == '\r' || c == '\n') {
            ide_insert_char('\n');
        } else if (c >= 32 || c == '\t') {
            ide_insert_char(c);
        }
    }

    vga_clear();
    vga_hide_hw_cursor();
    keyboard_flush();
}
