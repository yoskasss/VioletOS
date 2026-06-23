/* VioletOS - Interactive shell */
#include "violet.h"
#include "violet_lang.h"

#define SHELL_MAX_INPUT 256
#define SHELL_MAX_ARGS  16

extern void editor_open(const char* filename);
extern void sys_reboot(void);
extern void sys_shutdown(void);
extern const char* fs_get_cwd(void);
extern int fs_set_cwd(const char* path);

static void shell_print_prompt(void) {
    vga_set_color(13, 0);
    vga_puts("violet@os:");
    vga_set_color(15, 0);
    vga_puts(fs_get_cwd());
    vga_set_color(13, 0);
    vga_puts("$ ");
    vga_set_color(15, 0);
}

static int shell_parse(char* input, char** argv) {
    int argc = 0;
    while (*input && argc < SHELL_MAX_ARGS - 1) {
        while (*input == ' ') input++;
        if (!*input) break;
        argv[argc++] = input;
        while (*input && *input != ' ') input++;
        if (*input) { *input = '\0'; input++; }
    }
    argv[argc] = NULL;
    return argc;
}

static void cmd_help(void) {
    vga_puts("Available commands:\n");
    vga_puts("  help              - Show this help\n");
    vga_puts("  clear             - Clear screen\n");
    vga_puts("  ls                - List files\n");
    vga_puts("  pwd               - Print working directory\n");
    vga_puts("  mkdir <folder>    - Create directory\n");
    vga_puts("  touch <file>      - Create file\n");
    vga_puts("  rm <file>         - Delete file\n");
    vga_puts("  cat <file>        - Display file content\n");
    vga_puts("  not <filename>    - Open text editor\n");
    vga_puts("  ide               - Open Violet IDE\n");
    vga_puts("  run <file.v>      - Run Violet program\n");
    vga_puts("  fetch             - System information\n");
    vga_puts("  reboot            - Restart system\n");
    vga_puts("  shutdown          - Power off\n");
    vga_puts("  echo <text>       - Print text\n");
    vga_puts("  date              - Show date/time\n");
    vga_puts("  exit              - Return to main menu\n");
}

static void cmd_fetch(void) {
    char buf[32];
    vga_set_color(13, 0);
    vga_puts("       .---.\n");
    vga_puts("      /     \\\n");
    vga_puts("      \\.@-@./\n");
    vga_puts("       /\\_/\\\n");
    vga_set_color(15, 0);
    vga_puts("\n");
    vga_puts("VioletOS "); vga_puts(VIOLETOS_VERSION); vga_puts("\n");
    vga_puts("Kernel  "); vga_puts(KERNEL_VERSION); vga_puts("\n\n");
    vga_puts("Memory:\n");
    vga_puts("  Total:     "); itoa((int)get_total_memory(), buf, 10); vga_puts(buf); vga_puts(" bytes\n");
    vga_puts("  Used:      "); itoa((int)get_used_memory(), buf, 10); vga_puts(buf); vga_puts(" bytes\n");
    vga_puts("  Available: "); itoa((int)get_free_memory(), buf, 10); vga_puts(buf); vga_puts(" bytes\n\n");
    vga_puts("Storage:\n");
    vga_puts("  Used:  "); itoa((int)fs_get_used_storage(), buf, 10); vga_puts(buf); vga_puts(" bytes\n");
    vga_puts("  Total: "); itoa((int)fs_get_total_storage(), buf, 10); vga_puts(buf); vga_puts(" bytes\n\n");
    vga_puts("CPU:     x86 (32-bit) Intel-compatible\n");
    vga_puts("Uptime:  "); itoa((int)timer_get_uptime_seconds(), buf, 10); vga_puts(buf); vga_puts(" seconds\n");
}

static void cmd_date(void) {
    uint32_t s = timer_get_uptime_seconds();
    uint32_t h = s / 3600;
    uint32_t m = (s % 3600) / 60;
    uint32_t sec = s % 60;
    char buf[16];
    vga_puts("System uptime-based clock: ");
    itoa((int)h, buf, 10); vga_puts(buf); vga_puts(":");
    if (m < 10) vga_puts("0");
    itoa((int)m, buf, 10); vga_puts(buf); vga_puts(":");
    if (sec < 10) vga_puts("0");
    itoa((int)sec, buf, 10); vga_puts(buf); vga_puts("\n");
    vga_puts("(RTC not available - showing uptime as HH:MM:SS)\n");
}

static void shell_execute(int argc, char** argv) {
    if (argc == 0) return;

    if (strcmp(argv[0], "help") == 0) {
        cmd_help();
    } else if (strcmp(argv[0], "clear") == 0) {
        vga_clear();
    } else if (strcmp(argv[0], "ls") == 0) {
        char buf[2048];
        fs_ls(fs_get_cwd(), buf, sizeof(buf));
        vga_puts(buf);
    } else if (strcmp(argv[0], "pwd") == 0) {
        vga_puts(fs_get_cwd()); vga_puts("\n");
    } else if (strcmp(argv[0], "mkdir") == 0) {
        if (argc < 2) { vga_puts("Usage: mkdir <folder>\n"); return; }
        if (fs_mkdir(argv[1]) != 0) vga_puts("Error creating directory\n");
    } else if (strcmp(argv[0], "touch") == 0) {
        if (argc < 2) { vga_puts("Usage: touch <file>\n"); return; }
        if (fs_touch(argv[1]) != 0) vga_puts("Error creating file\n");
    } else if (strcmp(argv[0], "rm") == 0) {
        if (argc < 2) { vga_puts("Usage: rm <file>\n"); return; }
        if (fs_rm(argv[1]) != 0) vga_puts("Error deleting file\n");
    } else if (strcmp(argv[0], "cat") == 0) {
        if (argc < 2) { vga_puts("Usage: cat <file>\n"); return; }
        char buf[4096];
        if (fs_cat(argv[1], buf, sizeof(buf)) < 0)
            vga_puts("File not found\n");
        else
            vga_puts(buf);
    } else if (strcmp(argv[0], "not") == 0) {
        if (argc < 2) { vga_puts("Usage: not <filename>\n"); return; }
        editor_open(argv[1]);
        vga_clear();
    } else if (strcmp(argv[0], "ide") == 0) {
        vga_hide_hw_cursor();
        keyboard_flush();
        ide_run();
        vga_clear();
        vga_set_color(15, 0);
        vga_show_hw_cursor();

    } else if (strcmp(argv[0], "fetch") == 0) {
        cmd_fetch();
    } else if (strcmp(argv[0], "reboot") == 0) {
        sys_reboot();
    } else if (strcmp(argv[0], "shutdown") == 0) {
        sys_shutdown();
    } else if (strcmp(argv[0], "echo") == 0) {
        for (int i = 1; i < argc; i++) {
            vga_puts(argv[i]);
            if (i < argc - 1) vga_putchar(' ');
        }
        vga_putchar('\n');
    } else if (strcmp(argv[0], "date") == 0) {
        cmd_date();
    } else if (strcmp(argv[0], "run") == 0) {
        if (argc < 2) { vga_puts("Usage: run <file.v>\n"); return; }
        violet_run_file(argv[1]);
    
    } else if (strcmp(argv[0], "exit") == 0) {
        return;
    } else {
        vga_puts("Unknown command: ");
        vga_puts(argv[0]);
        vga_puts("\nType 'help' for available commands.\n");
    }
}

void shell_run(void) {
    char input[SHELL_MAX_INPUT];
    char* argv[SHELL_MAX_ARGS];

    vga_clear();
    vga_set_color(13, 0);
    vga_puts("VioletOS Terminal v");
    vga_puts(VIOLETOS_VERSION);
    vga_puts("\nType 'help' for commands, 'exit' to return to menu.\n\n");
    vga_set_color(15, 0);
    vga_show_hw_cursor();

    while (1) {
        shell_print_prompt();
        int prompt_row, prompt_col;
        vga_get_cursor(&prompt_row, &prompt_col);

        int pos = 0;
        memset(input, 0, sizeof(input));

        while (1) {
            vga_set_cursor(prompt_row, prompt_col + pos);
            char c = keyboard_getchar();
            if (c == KEY_ENTER || c == '\r' || c == '\n') {
                vga_putchar('\n');
                break;
            } else if (c == KEY_BS || c == 0x08) {
                if (pos > 0) {
                    pos--;
                    input[pos] = '\0';
                    vga_set_cursor(prompt_row, prompt_col + pos);
                    vga_putchar(' ');
                    vga_set_cursor(prompt_row, prompt_col + pos);
                }
            } else if (c >= 32 && pos < SHELL_MAX_INPUT - 1) {
                input[pos++] = c;
                vga_putchar(c);
            }
        }

        if (strcmp(input, "exit") == 0) break;

        int argc = shell_parse(input, argv);
        shell_execute(argc, argv);
    }

    vga_hide_hw_cursor();
}
