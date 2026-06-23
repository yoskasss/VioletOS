/* VioletOS - Violet programming language interpreter */
#include "violet_lang.h"
#include "violet.h"

#define MAX_VARS     64
#define MAX_LINE     256
#define MAX_LINES    256
#define MAX_STR      256

typedef enum {
    VTYPE_STRING,
    VTYPE_NUMBER,
    VTYPE_BOOL,
    VTYPE_NONE
} VType;

typedef struct {
    char name[32];
    VType type;
    char str[MAX_STR];
    int number;
    bool boolean;
} Variable;

static Variable vars[MAX_VARS];
static int var_count;

static char line_buf[MAX_LINES][MAX_LINE];
static int line_total;

static void vars_clear(void) {
    var_count = 0;
    memset(vars, 0, sizeof(vars));
}

static Variable* var_find(const char* name) {
    for (int i = 0; i < var_count; i++)
        if (strcmp(vars[i].name, name) == 0)
            return &vars[i];
    return NULL;
}

static Variable* var_create(const char* name) {
    Variable* v = var_find(name);
    if (v) return v;
    if (var_count >= MAX_VARS) return NULL;
    v = &vars[var_count++];
    strncpy(v->name, name, 31);
    v->type = VTYPE_NONE;
    return v;
}

static const char* skip_ws(const char* s) {
    while (*s == ' ' || *s == '\t') s++;
    return s;
}

static bool is_ident_start(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_';
}

static bool is_ident(char c) {
    return is_ident_start(c) || (c >= '0' && c <= '9');
}

static bool parse_ident(const char* s, char* out, size_t max) {
    s = skip_ws(s);
    if (!is_ident_start(*s)) return false;
    size_t i = 0;
    while (is_ident(*s) && i < max - 1)
        out[i++] = *s++;
    out[i] = '\0';
    return i > 0;
}

static bool parse_string_lit(const char* s, char* out, size_t max) {
    s = skip_ws(s);
    if (*s != '"') return false;
    s++;
    size_t i = 0;
    while (*s && *s != '"') {
        if (i >= max - 1) return false;
        out[i++] = *s++;
    }
    if (*s != '"') return false;
    out[i] = '\0';
    return true;
}

static bool parse_number_lit(const char* s, int* out) {
    s = skip_ws(s);
    bool neg = false;
    if (*s == '-') { neg = true; s++; }
    if (*s < '0' || *s > '9') return false;
    int val = 0;
    while (*s >= '0' && *s <= '9')
        val = val * 10 + (*s++ - '0');
    *out = neg ? -val : val;
    return true;
}

static bool parse_bool_lit(const char* s, bool* out) {
    s = skip_ws(s);
    if (strncmp(s, "true", 4) == 0 && !is_ident(s[4])) { *out = true; return true; }
    if (strncmp(s, "false", 5) == 0 && !is_ident(s[5])) { *out = false; return true; }
    return false;
}

static bool is_block_start(const char* line) {
    return strncmp(line, "if ", 3) == 0 ||
           strncmp(line, "loop ", 5) == 0 ||
           strncmp(line, "while ", 6) == 0;
}

static bool is_block_end(const char* line) {
    return strncmp(line, "end", 3) == 0 &&
           (line[3] == '\0' || line[3] == ' ');
}

static bool is_else_line(const char* line) {
    return strncmp(line, "else", 4) == 0 &&
           (line[4] == '\0' || line[4] == ' ');
}

static void value_print(const Variable* v) {
    if (!v || v->type == VTYPE_NONE) return;
    if (v->type == VTYPE_STRING) vga_puts(v->str);
    else if (v->type == VTYPE_NUMBER) {
        char b[16]; itoa(v->number, b, 10); vga_puts(b);
    } else if (v->type == VTYPE_BOOL)
        vga_puts(v->boolean ? "true" : "false");
}

static bool has_math_ops(const char* expr) {
    bool in_str = false;
    for (int i = 0; expr[i]; i++) {
        if (expr[i] == '"') in_str = !in_str;
        if (in_str) continue;
        if (expr[i] == '*' || expr[i] == '/') return true;
        if ((expr[i] == '+' || expr[i] == '-') && i > 0) {
            const char* p = skip_ws(expr + i + 1);
            if (*p && ((*p >= '0' && *p <= '9') || is_ident_start(*p)))
                return true;
        }
    }
    return false;
}

static bool has_plus_outside_string(const char* expr) {
    bool in_str = false;
    for (int i = 0; expr[i]; i++) {
        if (expr[i] == '"') in_str = !in_str;
        if (!in_str && expr[i] == '+' && i > 0) return true;
    }
    return false;
}

static bool eval_number_expr(const char* expr, int* result);
static bool eval_value(const char* expr, Variable* out);

static bool eval_add_expr(const char* expr, Variable* out) {
    bool in_str = false;
    for (int i = 0; expr[i]; i++) {
        if (expr[i] == '"') in_str = !in_str;
        if (in_str || expr[i] != '+' || i == 0) continue;

        char left[MAX_LINE], right[MAX_LINE];
        strncpy(left, expr, (size_t)i);
        left[i] = '\0';
        strcpy(right, expr + i + 1);

        Variable lv, rv;
        if (!eval_value(left, &lv) || !eval_value(right, &rv)) return false;

        if (lv.type == VTYPE_STRING && rv.type == VTYPE_STRING) {
            out->type = VTYPE_STRING;
            strcpy(out->str, lv.str);
            strcat(out->str, rv.str);
            return true;
        }
        if (lv.type == VTYPE_NUMBER && rv.type == VTYPE_NUMBER) {
            out->type = VTYPE_NUMBER;
            out->number = lv.number + rv.number;
            return true;
        }
        return false;
    }
    return false;
}

static bool eval_value(const char* expr, Variable* out) {
    char ident[32];
    char str[MAX_STR];
    int num;
    bool b;

    expr = skip_ws(expr);
    if (!*expr) return false;

    if (parse_string_lit(expr, str, MAX_STR)) {
        out->type = VTYPE_STRING;
        strcpy(out->str, str);
        return true;
    }
    if (parse_bool_lit(expr, &b)) {
        out->type = VTYPE_BOOL;
        out->boolean = b;
        return true;
    }
    if (has_plus_outside_string(expr) && eval_add_expr(expr, out))
        return true;
    if (has_math_ops(expr) && eval_number_expr(expr, &num)) {
        out->type = VTYPE_NUMBER;
        out->number = num;
        return true;
    }
    if (parse_number_lit(expr, &num)) {
        out->type = VTYPE_NUMBER;
        out->number = num;
        return true;
    }
    if (parse_ident(expr, ident, sizeof(ident))) {
        Variable* v = var_find(ident);
        if (!v || v->type == VTYPE_NONE) return false;
        *out = *v;
        return true;
    }
    return false;
}

static int get_number_val(const char* token) {
    char ident[32];
    int num;
    if (parse_number_lit(token, &num)) return num;
    if (parse_ident(token, ident, sizeof(ident))) {
        Variable* v = var_find(ident);
        if (v && v->type == VTYPE_NUMBER) return v->number;
    }
    return 0;
}

static bool eval_number_expr(const char* expr, int* result) {
    char work[MAX_LINE];
    strncpy(work, expr, MAX_LINE - 1);
    work[MAX_LINE - 1] = '\0';

    for (int i = 0; work[i]; i++) {
        if (work[i] == '*' || work[i] == '/') {
            int start = i - 1;
            while (start > 0 && work[start - 1] == ' ') start--;
            while (start > 0 && (is_ident(work[start - 1]) || work[start - 1] == '-')) start--;
            int left_start = start;
            while (work[left_start] == ' ') left_start++;

            int j = i + 1;
            while (work[j] == ' ') j++;

            char lt[32], rt[32];
            int li = 0;
            while (work[left_start] && work[left_start] != ' ' &&
                   work[left_start] != '*' && work[left_start] != '/' &&
                   work[left_start] != '+' && work[left_start] != '-')
                lt[li++] = work[left_start++];
            lt[li] = '\0';

            int ri = 0;
            while (work[j] && work[j] != ' ' &&
                   work[j] != '*' && work[j] != '/' &&
                   work[j] != '+' && work[j] != '-')
                rt[ri++] = work[j++];
            rt[ri] = '\0';

            int lv = get_number_val(lt);
            int rv = get_number_val(rt);
            int val = (work[i] == '*') ? lv * rv : (rv != 0 ? lv / rv : 0);

            char repl[32];
            itoa(val, repl, 10);
            char newwork[MAX_LINE];
            int p = 0;
            for (int k = 0; k < left_start && work[k]; k++)
                newwork[p++] = work[k];
            for (int k = 0; repl[k]; k++)
                newwork[p++] = repl[k];
            for (int k = j; work[k]; k++)
                newwork[p++] = work[k];
            newwork[p] = '\0';
            strcpy(work, newwork);
            i = 0;
        }
    }

    int acc = 0;
    char token[32];
    char op = '+';
    const char* p = skip_ws(work);

    while (1) {
        while (*p == ' ') p++;
        if (!*p) break;
        int ti = 0;
        while (*p && *p != ' ' && *p != '+' && *p != '-')
            token[ti++] = *p++;
        token[ti] = '\0';
        int val = get_number_val(token);
        if (op == '+') acc += val;
        else acc -= val;
        while (*p == ' ') p++;
        if (*p == '+' || *p == '-') { op = *p++; continue; }
        break;
    }
    *result = acc;
    return true;
}

static bool eval_simple_condition(const char* expr);

static bool eval_and_chain(const char* expr) {
    char part[MAX_LINE];
    const char* p = expr;

    while (1) {
        const char* and_pos = NULL;
        bool in_str = false;
        for (const char* q = p; *q; q++) {
            if (*q == '"') in_str = !in_str;
            if (!in_str && strncmp(q, " and ", 5) == 0) { and_pos = q; break; }
        }

        if (and_pos) {
            size_t len = (size_t)(and_pos - p);
            if (len >= MAX_LINE) len = MAX_LINE - 1;
            strncpy(part, p, len);
            part[len] = '\0';
            if (!eval_simple_condition(part)) return false;
            p = and_pos + 5;
        } else {
            return eval_simple_condition(p);
        }
    }
}

static bool eval_condition(const char* expr) {
    char part[MAX_LINE];
    const char* p = skip_ws(expr);

    while (1) {
        const char* or_pos = NULL;
        bool in_str = false;
        for (const char* q = p; *q; q++) {
            if (*q == '"') in_str = !in_str;
            if (!in_str && strncmp(q, " or ", 4) == 0) { or_pos = q; break; }
        }

        if (or_pos) {
            size_t len = (size_t)(or_pos - p);
            if (len >= MAX_LINE) len = MAX_LINE - 1;
            strncpy(part, p, len);
            part[len] = '\0';
            if (eval_and_chain(part)) return true;
            p = or_pos + 4;
        } else {
            return eval_and_chain(p);
        }
    }
}

static bool eval_simple_condition(const char* expr) {
    char left[MAX_STR], right[MAX_STR], op[3];
    const char* p = skip_ws(expr);
    char ident[32];
    bool bval;

    if (strncmp(p, "not ", 4) == 0)
        return !eval_simple_condition(p + 4);

    if (parse_bool_lit(p, &bval)) return bval;

    if (*p == '"') {
        parse_string_lit(p, left, MAX_STR);
        p = strchr(p, '"');
        if (p) p++;
    } else if ((*p >= '0' && *p <= '9') || *p == '-') {
        int n; parse_number_lit(p, &n);
        itoa(n, left, 10);
        while (*p && *p != ' ') p++;
    } else {
        parse_ident(p, ident, sizeof(ident));
        Variable* v = var_find(ident);
        if (!v || v->type == VTYPE_NONE) return false;
        if (v->type == VTYPE_STRING) strcpy(left, v->str);
        else if (v->type == VTYPE_NUMBER) itoa(v->number, left, 10);
        else if (v->type == VTYPE_BOOL) return v->boolean;
        while (*p && *p != ' ') p++;
    }

    p = skip_ws(p);
    if (strncmp(p, "==", 2) == 0) { strcpy(op, "=="); p += 2; }
    else if (strncmp(p, "!=", 2) == 0) { strcpy(op, "!="); p += 2; }
    else if (strncmp(p, ">=", 2) == 0) { strcpy(op, ">="); p += 2; }
    else if (strncmp(p, "<=", 2) == 0) { strcpy(op, "<="); p += 2; }
    else if (*p == '>') { strcpy(op, ">"); p++; }
    else if (*p == '<') { strcpy(op, "<"); p++; }
    else {
        if (strcmp(left, "true") == 0) return true;
        if (strcmp(left, "false") == 0) return false;
        return atoi(left) != 0;
    }

    p = skip_ws(p);
    if (*p == '"') parse_string_lit(p, right, MAX_STR);
    else if ((*p >= '0' && *p <= '9') || *p == '-') {
        int n; parse_number_lit(p, &n); itoa(n, right, 10);
    } else {
        parse_ident(p, ident, sizeof(ident));
        Variable* v = var_find(ident);
        if (!v || v->type == VTYPE_NONE) return false;
        if (v->type == VTYPE_STRING) strcpy(right, v->str);
        else if (v->type == VTYPE_NUMBER) itoa(v->number, right, 10);
        else strcpy(right, v->boolean ? "true" : "false");
    }

    int ln = atoi(left), rn = atoi(right);
    bool use_num = (left[0] >= '0' && left[0] <= '9') || left[0] == '-';

    if (strcmp(op, "==") == 0) {
        if (use_num) return ln == rn;
        return strcmp(left, right) == 0;
    }
    if (strcmp(op, "!=") == 0) {
        if (use_num) return ln != rn;
        return strcmp(left, right) != 0;
    }
    if (strcmp(op, ">") == 0) return ln > rn;
    if (strcmp(op, "<") == 0) return ln < rn;
    if (strcmp(op, ">=") == 0) return ln >= rn;
    if (strcmp(op, "<=") == 0) return ln <= rn;
    return false;
}

static int find_matching_end(int from) {
    int depth = 1;
    for (int i = from + 1; i < line_total; i++) {
        const char* l = skip_ws(line_buf[i]);
        if (is_block_start(l)) depth++;
        else if (is_block_end(l)) {
            depth--;
            if (depth == 0) return i;
        }
    }
    return line_total;
}

static int find_else_line(int start, int end) {
    int depth = 0;
    for (int i = start; i < end; i++) {
        const char* l = skip_ws(line_buf[i]);
        if (is_block_start(l)) depth++;
        else if (is_block_end(l)) depth--;
        else if (depth == 0 && is_else_line(l)) return i;
    }
    return -1;
}

static int parse_loop_count(const char* rest, int* errline, int idx) {
    int count = 0;
    char ident[32];
    rest = skip_ws(rest);
    if (parse_number_lit(rest, &count)) return count;
    if (parse_ident(rest, ident, sizeof(ident))) {
        Variable* v = var_find(ident);
        if (v && v->type == VTYPE_NUMBER) return v->number;
    }
    *errline = idx;
    return -1;
}

static void input_store(const char* name, const char* buf) {
    Variable* v = var_create(name);
    if (!v) return;

    bool b;
    int num;
    if (parse_bool_lit(buf, &b)) {
        v->type = VTYPE_BOOL;
        v->boolean = b;
    } else if (parse_number_lit(buf, &num)) {
        v->type = VTYPE_NUMBER;
        v->number = num;
    } else {
        v->type = VTYPE_STRING;
        strncpy(v->str, buf, MAX_STR - 1);
    }
}

static int execute_range(int start, int end, int* errline);

static int execute_line(int idx, int* errline) {
    const char* line = skip_ws(line_buf[idx]);
    if (!*line) return 0;
    if (*line == '#' || strncmp(line, "//", 2) == 0) return 0;

    if (strncmp(line, "print ", 6) == 0) {
        Variable v;
        if (!eval_value(line + 6, &v)) { *errline = idx; return -1; }
        value_print(&v);
        vga_putchar('\n');
        return 0;
    }

    if (strcmp(line, "clear") == 0) {
        vga_clear();
        return 0;
    }

    if (strncmp(line, "set ", 4) == 0) {
        char name[32];
        const char* rest = skip_ws(line + 4);
        if (!parse_ident(rest, name, sizeof(name))) { *errline = idx; return -1; }
        rest = skip_ws(rest + strlen(name));
        Variable* v = var_create(name);
        if (!v) { *errline = idx; return -1; }
        if (!eval_value(rest, v)) { *errline = idx; return -1; }
        return 0;
    }

    if (strncmp(line, "input ", 6) == 0) {
        char name[32];
        if (!parse_ident(line + 6, name, sizeof(name))) { *errline = idx; return -1; }
        vga_puts("? ");
        char buf[MAX_STR];
        int pos = 0;
        while (1) {
            char c = keyboard_getchar();
            if (c == KEY_ENTER || c == '\r' || c == '\n') { vga_putchar('\n'); break; }
            if (c == KEY_BS && pos > 0) { pos--; vga_putchar('\b'); }
            else if (c >= 32 && pos < MAX_STR - 1) { buf[pos++] = c; vga_putchar(c); }
        }
        buf[pos] = '\0';
        input_store(name, buf);
        return 0;
    }

    if (strncmp(line, "if ", 3) == 0) {
        bool ok = eval_condition(line + 3);
        int end = find_matching_end(idx);
        int else_line = find_else_line(idx + 1, end);
        if (ok) {
            int stop = (else_line >= 0) ? else_line : end;
            if (execute_range(idx + 1, stop, errline) < 0) return -1;
        } else if (else_line >= 0) {
            if (execute_range(else_line + 1, end, errline) < 0) return -1;
        }
        return end - idx;
    }

    if (strncmp(line, "while ", 6) == 0) {
        int end = find_matching_end(idx);
        const char* cond = line + 6;
        while (eval_condition(cond)) {
            if (execute_range(idx + 1, end, errline) < 0) return -1;
        }
        return end - idx;
    }

    if (strncmp(line, "loop ", 5) == 0) {
        int count = parse_loop_count(line + 5, errline, idx);
        if (count < 0) return -1;
        int end = find_matching_end(idx);
        for (int i = 0; i < count; i++)
            if (execute_range(idx + 1, end, errline) < 0) return -1;
        return end - idx;
    }

    if (is_block_end(line)) return 0;

    *errline = idx;
    return -1;
}

static int execute_range(int start, int end, int* errline) {
    for (int i = start; i < end; i++) {
        int skip = execute_line(i, errline);
        if (skip < 0) return -1;
        if (skip > 0) i += skip;
    }
    return 0;
}

static bool split_source(const char* source) {
    line_total = 0;
    const char* p = source;
    while (*p && line_total < MAX_LINES) {
        int i = 0;
        while (*p && *p != '\n' && i < MAX_LINE - 1)
            line_buf[line_total][i++] = *p++;
        line_buf[line_total][i] = '\0';
        line_total++;
        if (*p == '\n') p++;
    }
    return line_total > 0;
}

static void report_error(int errline) {
    vga_set_color(12, 0);
    vga_puts("Violet error on line ");
    char b[8]; itoa(errline + 1, b, 10);
    vga_puts(b);
    vga_puts(": ");
    vga_puts(line_buf[errline]);
    vga_putchar('\n');
    vga_set_color(15, 0);
}

int violet_run_source(const char* source) {
    if (!source) return -1;
    vars_clear();
    if (!split_source(source)) return -1;

    int errline = -1;
    if (execute_range(0, line_total, &errline) < 0) {
        if (errline >= 0) report_error(errline);
        else {
            vga_set_color(12, 0);
            vga_puts("Violet error\n");
            vga_set_color(15, 0);
        }
        return -1;
    }
    return 0;
}

int violet_run_file(const char* path) {
    char buf[4096];
    if (fs_read(path, buf, sizeof(buf)) < 0) {
        vga_puts("Cannot open file: ");
        vga_puts(path);
        vga_putchar('\n');
        return -1;
    }
    return violet_run_source(buf);
}
