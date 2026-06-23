/* VioletOS - PS/2 Keyboard driver with Turkish Q layout */
#include "violet.h"
#include "port.h"

#define KBD_DATA_PORT    0x60
#define KBD_STATUS_PORT  0x64
#define KBD_BUFFER_SIZE  256

/* CP857 Turkish character codes */
#define TR_c_Ccedilla 0x80
#define TR_c_ccedilla 0x87
#define TR_c_Idotless 0x8D
#define TR_c_odieresis 0x94
#define TR_c_Odieresis 0x99
#define TR_c_udieresis 0x81
#define TR_c_Udieresis 0x9A
#define TR_c_Scedilla 0x9E
#define TR_c_scedilla 0x9F
#define TR_c_Gbreve   0xA6
#define TR_c_gbreve   0xA7
#define TR_c_Idot     0x98

static char key_buffer[KBD_BUFFER_SIZE];
static int  buffer_head = 0;
static int  buffer_tail = 0;
static bool shift_pressed = false;
static bool ctrl_pressed = false;
static bool caps_lock = false;
static bool extended = false;
static uint8_t last_scancode = 0;
static bool key_down[128];
static bool ext_key_down[128];

/* Scancode set 1 lookup tables indexed by scancode (0x00-0x7F) */
static char tr_q_normal[128];
static char tr_q_shift[128];

static void keyboard_build_tables(void) {
    memset(tr_q_normal, 0, 128);
    memset(tr_q_shift, 0, 128);

    tr_q_normal[0x01] = 27;
    tr_q_normal[0x02] = '1';  tr_q_shift[0x02] = '!';
    tr_q_normal[0x03] = '2';  tr_q_shift[0x03] = '\'';
    tr_q_normal[0x04] = '3';  tr_q_shift[0x04] = '^';
    tr_q_normal[0x05] = '4';  tr_q_shift[0x05] = '+';
    tr_q_normal[0x06] = '5';  tr_q_shift[0x06] = '%';
    tr_q_normal[0x07] = '6';  tr_q_shift[0x07] = '&';
    tr_q_normal[0x08] = '7';  tr_q_shift[0x08] = '/';
    tr_q_normal[0x09] = '8';  tr_q_shift[0x09] = '(';
    tr_q_normal[0x0A] = '9';  tr_q_shift[0x0A] = ')';
    tr_q_normal[0x0B] = '0';  tr_q_shift[0x0B] = '=';
    tr_q_normal[0x0C] = '*';  tr_q_shift[0x0C] = '?';
    tr_q_normal[0x0D] = '-';  tr_q_shift[0x0D] = '_';
    tr_q_normal[0x0E] = '\b';
    tr_q_normal[0x0F] = '\t';

    tr_q_normal[0x10] = 'q';  tr_q_shift[0x10] = 'Q';
    tr_q_normal[0x11] = 'w';  tr_q_shift[0x11] = 'W';
    tr_q_normal[0x12] = 'e';  tr_q_shift[0x12] = 'E';
    tr_q_normal[0x13] = 'r';  tr_q_shift[0x13] = 'R';
    tr_q_normal[0x14] = 't';  tr_q_shift[0x14] = 'T';
    tr_q_normal[0x15] = 'y';  tr_q_shift[0x15] = 'Y';
    tr_q_normal[0x16] = 'u';  tr_q_shift[0x16] = 'U';
    tr_q_normal[0x17] = TR_c_Idotless; tr_q_shift[0x17] = TR_c_Idot;
    tr_q_normal[0x18] = 'o';  tr_q_shift[0x18] = 'O';
    tr_q_normal[0x19] = 'p';  tr_q_shift[0x19] = 'P';
    tr_q_normal[0x1A] = TR_c_gbreve;   tr_q_shift[0x1A] = TR_c_Gbreve;
    tr_q_normal[0x1B] = TR_c_udieresis; tr_q_shift[0x1B] = TR_c_Udieresis;
    tr_q_normal[0x1C] = '\n';

    tr_q_normal[0x1E] = 'a';  tr_q_shift[0x1E] = 'A';
    tr_q_normal[0x1F] = 's';  tr_q_shift[0x1F] = 'S';
    tr_q_normal[0x20] = 'd';  tr_q_shift[0x20] = 'D';
    tr_q_normal[0x21] = 'f';  tr_q_shift[0x21] = 'F';
    tr_q_normal[0x22] = 'g';  tr_q_shift[0x22] = 'G';
    tr_q_normal[0x23] = 'h';  tr_q_shift[0x23] = 'H';
    tr_q_normal[0x24] = 'j';  tr_q_shift[0x24] = 'J';
    tr_q_normal[0x25] = 'k';  tr_q_shift[0x25] = 'K';
    tr_q_normal[0x26] = 'l';  tr_q_shift[0x26] = 'L';
    tr_q_normal[0x27] = TR_c_scedilla; tr_q_shift[0x27] = TR_c_Scedilla;
    tr_q_normal[0x28] = 'i';  tr_q_shift[0x28] = TR_c_Idot;
    tr_q_normal[0x29] = '"';  tr_q_shift[0x29] = 0xE9; /* e-acute CP857 */

    tr_q_normal[0x2B] = '<';  tr_q_shift[0x2B] = '>';
    tr_q_normal[0x2C] = 'z';  tr_q_shift[0x2C] = 'Z';
    tr_q_normal[0x2D] = 'x';  tr_q_shift[0x2D] = 'X';
    tr_q_normal[0x2E] = 'c';  tr_q_shift[0x2E] = 'C';
    tr_q_normal[0x2F] = 'v';  tr_q_shift[0x2F] = 'V';
    tr_q_normal[0x30] = 'b';  tr_q_shift[0x30] = 'B';
    tr_q_normal[0x31] = 'n';  tr_q_shift[0x31] = 'N';
    tr_q_normal[0x32] = 'm';  tr_q_shift[0x32] = 'M';
    tr_q_normal[0x33] = TR_c_odieresis; tr_q_shift[0x33] = TR_c_Odieresis;
    tr_q_normal[0x34] = TR_c_ccedilla;  tr_q_shift[0x34] = TR_c_Ccedilla;
    tr_q_normal[0x35] = '.';  tr_q_shift[0x35] = ':';
    tr_q_normal[0x39] = ' ';
}

static void buffer_push(char c) {
    int next = (buffer_head + 1) % KBD_BUFFER_SIZE;
    if (next != buffer_tail) {
        key_buffer[buffer_head] = c;
        buffer_head = next;
    }
}

static char apply_caps(unsigned char c, bool shift) {
    if (c >= 'a' && c <= 'z')
        return (caps_lock ^ shift) ? (char)(c - 32) : (char)c;
    if (c >= 'A' && c <= 'Z')
        return (caps_lock ^ shift) ? (char)(c + 32) : (char)c;
    if (c == TR_c_Idotless && (caps_lock ^ shift)) return TR_c_Idot;
    if (c == TR_c_Idot && (caps_lock ^ shift)) return TR_c_Idotless;
    if (c == TR_c_gbreve && (caps_lock ^ shift)) return TR_c_Gbreve;
    if (c == TR_c_Gbreve && (caps_lock ^ shift)) return TR_c_gbreve;
    if (c == TR_c_udieresis && (caps_lock ^ shift)) return TR_c_Udieresis;
    if (c == TR_c_Udieresis && (caps_lock ^ shift)) return TR_c_udieresis;
    if (c == TR_c_scedilla && (caps_lock ^ shift)) return TR_c_Scedilla;
    if (c == TR_c_Scedilla && (caps_lock ^ shift)) return TR_c_scedilla;
    if (c == TR_c_odieresis && (caps_lock ^ shift)) return TR_c_Odieresis;
    if (c == TR_c_Odieresis && (caps_lock ^ shift)) return TR_c_odieresis;
    if (c == TR_c_ccedilla && (caps_lock ^ shift)) return TR_c_Ccedilla;
    if (c == TR_c_Ccedilla && (caps_lock ^ shift)) return TR_c_ccedilla;
    return (char)c;
}

static char scancode_to_char(uint8_t sc) {
    /* US/QEMU punctuation keys (same scancodes, layout-independent) */
    if (sc == 0x33) return shift_pressed ? '<' : ',';
    if (sc == 0x34) return shift_pressed ? '>' : '.';
    if (sc == 0x35) return shift_pressed ? '?' : '/';

    char c = shift_pressed ? tr_q_shift[sc] : tr_q_normal[sc];
    if (!c) return 0;

    if (ctrl_pressed && sc >= 0x10 && sc <= 0x35) {
        char base = tr_q_normal[sc];
        if (base >= 'a' && base <= 'z') return (char)(base - 'a' + 1);
        if (base >= 'A' && base <= 'Z') return (char)(base - 'A' + 1);
    }

    if (shift_pressed)
        return c;
    return apply_caps((unsigned char)c, false);
}

static void handle_ext_scancode(uint8_t sc, bool released) {
    if (released) {
        ext_key_down[sc] = false;
        return;
    }
    ext_key_down[sc] = true;
    switch (sc) {
        case 0x48: buffer_push((char)KEY_UP);    break;
        case 0x50: buffer_push((char)KEY_DOWN);  break;
        case 0x4B: buffer_push((char)KEY_LEFT);  break;
        case 0x4D: buffer_push((char)KEY_RIGHT); break;
    }
}

void keyboard_handler(void) {
    uint8_t status = inb(KBD_STATUS_PORT);
    if (status & 0x20) {
        inb(KBD_DATA_PORT);
        pic_eoi(1);
        return;
    }

    uint8_t sc = inb(KBD_DATA_PORT);
    last_scancode = sc;

    if (sc == 0xE0) {
        extended = true;
        pic_eoi(1);
        return;
    }

    bool released = sc & 0x80;
    sc &= 0x7F;

    if (extended) {
        extended = false;
        handle_ext_scancode(sc, released);
        pic_eoi(1);
        return;
    }

    if (sc == 0x2A || sc == 0x36) {
        shift_pressed = !released;
        pic_eoi(1);
        return;
    }
    if (sc == 0x1D) {
        ctrl_pressed = !released;
        pic_eoi(1);
        return;
    }
    if (sc == 0x3A && !released) {
        caps_lock = !caps_lock;
        pic_eoi(1);
        return;
    }

    key_down[sc] = !released;

    if (released) {
        pic_eoi(1);
        return;
    }

    char c = scancode_to_char(sc);
    if (c) {
        if (c == '\n') c = KEY_ENTER;
        buffer_push(c);
    }
    pic_eoi(1);
}

void keyboard_init(void) {
    keyboard_build_tables();
    buffer_head = buffer_tail = 0;
    shift_pressed = false;
    ctrl_pressed = false;
    caps_lock = false;
    memset(key_down, 0, sizeof(key_down));
    memset(ext_key_down, 0, sizeof(ext_key_down));
    while (inb(KBD_STATUS_PORT) & 1)
        inb(KBD_DATA_PORT);
}

bool keyboard_has_char(void) {
    return buffer_head != buffer_tail;
}

char keyboard_getchar(void) {
    while (buffer_head == buffer_tail)
        __asm__ volatile ("sti; hlt");
    char c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KBD_BUFFER_SIZE;
    return c;
}

bool keyboard_poll(char* c) {
    if (buffer_head == buffer_tail) return false;
    *c = key_buffer[buffer_tail];
    buffer_tail = (buffer_tail + 1) % KBD_BUFFER_SIZE;
    return true;
}

bool keyboard_key_down(uint8_t sc) {
    return sc < 128 && key_down[sc];
}

bool keyboard_ext_down(uint8_t sc) {
    return sc < 128 && ext_key_down[sc];
}

void keyboard_flush(void) {
    buffer_head = buffer_tail = 0;
    memset(key_down, 0, sizeof(key_down));
    memset(ext_key_down, 0, sizeof(ext_key_down));
    while (inb(KBD_STATUS_PORT) & 1)
        inb(KBD_DATA_PORT);
}

void keyboard_wait_release(void) {
    keyboard_flush();
    delay_ms(80);
    keyboard_flush();
}

bool keyboard_is_shift(void)  { return shift_pressed; }
bool keyboard_is_caps(void)   { return caps_lock; }
uint8_t keyboard_get_scancode(void) { return last_scancode; }
