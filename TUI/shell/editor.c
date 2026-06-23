/* VioletOS - Built-in text editor (not command) */
#include "violet.h"

#define EDITOR_MAX_LINES 64
#define EDITOR_MAX_LINE  80
#define EDITOR_MAX_CHARS (EDITOR_MAX_LINES * EDITOR_MAX_LINE)

static char editor_buffer[EDITOR_MAX_CHARS];
static int  editor_lines[EDITOR_MAX_LINES];
static int  line_count = 1;
static int  cursor_line = 0;
static int  cursor_col = 0;
static char editor_filename[FS_MAX_NAME];


static void editor_init_buffer(void) {
    memset(editor_buffer, 0, sizeof(editor_buffer));
    memset(editor_lines, 0, sizeof(editor_lines));
    line_count = 1;
    cursor_line = 0;
    cursor_col = 0;
    editor_lines[0] = 0;
}

static int editor_line_start(int line) {
    return editor_lines[line];
}

static int editor_line_len(int line) {
    if (line >= line_count) return 0;
    int start = editor_lines[line];
    int end = (line + 1 < line_count) ? editor_lines[line + 1] - 1 : (int)strlen(editor_buffer);
    return end - start;
}

static void editor_insert_char(char c) {
    int pos = editor_line_start(cursor_line) + cursor_col;
    int len = (int)strlen(editor_buffer);
    if (len >= EDITOR_MAX_CHARS - 2) return;

    if (c == '\n') {
        memmove(editor_buffer + pos + 1, editor_buffer + pos, len - pos + 1);
        editor_buffer[pos] = '\n';
        for (int i = line_count; i > cursor_line + 1; i--)
            editor_lines[i] = editor_lines[i - 1] + 1;
        editor_lines[cursor_line + 1] = pos + 1;
        line_count++;
        cursor_line++;
        cursor_col = 0;
        return;
    }

    memmove(editor_buffer + pos + 1, editor_buffer + pos, len - pos + 1);
    editor_buffer[pos] = c;
    for (int i = cursor_line + 1; i < line_count; i++)
        editor_lines[i]++;
    cursor_col++;
}

static void editor_delete_char(void) {
    if (cursor_col > 0) {
        int pos = editor_line_start(cursor_line) + cursor_col - 1;
        int len = (int)strlen(editor_buffer);
        memmove(editor_buffer + pos, editor_buffer + pos + 1, len - pos);
        for (int i = cursor_line + 1; i < line_count; i++)
            editor_lines[i]--;
        cursor_col--;
    } else if (cursor_line > 0) {
        int prev_len = editor_line_len(cursor_line - 1);
        int pos = editor_line_start(cursor_line);
        int len = editor_line_len(cursor_line);
        memmove(editor_buffer + pos - 1, editor_buffer + pos, len + 1);
        for (int i = cursor_line; i < line_count - 1; i++)
            editor_lines[i] = editor_lines[i + 1] - len - 1;
        line_count--;
        cursor_line--;
        cursor_col = prev_len;
    }
}

static void editor_render(void) {
    vga_clear();
    vga_set_color(13, 0);
    vga_puts("VioletOS Editor - ");
    vga_puts(editor_filename);
    vga_puts("  |  Ctrl+O Save  Ctrl+Q Quit\n");
    vga_set_color(15, 0);
    vga_puts("----------------------------------------\n");

    for (int i = 0; i < line_count && i < 20; i++) {
        int start = editor_line_start(i);
        int len = editor_line_len(i);
        for (int j = 0; j < len && j < 78; j++)
            vga_putchar(editor_buffer[start + j]);
        vga_putchar('\n');
    }

    int row = 3 + cursor_line;
    int col = cursor_col;
    if (row < 23) vga_set_cursor(row, col);
}

static void editor_save(void) {
    fs_write(editor_filename, editor_buffer, strlen(editor_buffer));
    vga_set_cursor(23, 0);
    vga_set_color(10, 0);
    vga_puts("File saved.");
    vga_set_color(15, 0);
    delay_ms(500);
}

static void editor_rebuild_lines(void) {
    line_count = 0;
    editor_lines[0] = 0;
    int len = (int)strlen(editor_buffer);
    for (int i = 0; i < len; i++) {
        if (editor_buffer[i] == '\n' && line_count < EDITOR_MAX_LINES - 1) {
            line_count++;
            editor_lines[line_count] = i + 1;
        }
    }
    line_count++;
}

void editor_open(const char* filename) {
    editor_init_buffer();
    strncpy(editor_filename, filename, FS_MAX_NAME - 1);

    char buf[EDITOR_MAX_CHARS];
    if (fs_read(filename, buf, sizeof(buf)) > 0) {
        strcpy(editor_buffer, buf);
        editor_rebuild_lines();
    }

    while (1) {
        editor_render();
        char c = keyboard_getchar();

        if (c == 0x11) { /* Ctrl+Q */
            break;
        } else if (c == 0x0F) { /* Ctrl+O */
            editor_save();
        } else if (c == (char)0xF0) { /* UP */
            if (cursor_line > 0) cursor_line--;
            if (cursor_col > editor_line_len(cursor_line))
                cursor_col = editor_line_len(cursor_line);
        } else if (c == (char)0xF1) { /* DOWN */
            if (cursor_line < line_count - 1) cursor_line++;
            if (cursor_col > editor_line_len(cursor_line))
                cursor_col = editor_line_len(cursor_line);
        } else if (c == (char)0xF2) { /* LEFT */
            if (cursor_col > 0) cursor_col--;
            else if (cursor_line > 0) {
                cursor_line--;
                cursor_col = editor_line_len(cursor_line);
            }
        } else if (c == (char)0xF3) { /* RIGHT */
            if (cursor_col < editor_line_len(cursor_line)) cursor_col++;
            else if (cursor_line < line_count - 1) {
                cursor_line++;
                cursor_col = 0;
            }
        } else if (c == 0x08 || c == 0x7F) {
            editor_delete_char();
        } else if (c >= 32 || c == '\n' || c == '\t') {
            editor_insert_char(c);
        }
    }
}
