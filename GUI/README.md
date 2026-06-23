# VioletOS

**VioletOS** is a complete educational 32-bit x86 operating system written in C and Assembly. It features a text-based GUI, Turkish Q keyboard support, a virtual filesystem with persistent storage, an interactive shell with a visible cursor, the **Violet IDE** for writing `.v` programs, and the **Violet** programming language with a built-in interpreter.

## Features

- GRUB multiboot kernel (x86 32-bit)
- GDT, IDT, PIC IRQ handling
- VGA text mode (purple theme) with hardware and software cursors
- PS/2 keyboard driver with **Turkish Q** layout
- PIT timer (100 Hz)
- ATA PIO disk driver for filesystem persistence
- VioletFS — simple in-kernel filesystem
- Interactive shell with 15+ commands and blinking input cursor
- **Violet IDE** — line numbers, active-line highlight, visible cursor
- **Violet language** — interpreted `.v` scripts (`print`, `set`, `if`/`else`, `while`, `loop`, math, strings)

## Requirements

| Tool | Purpose |
|------|---------|
| `gcc` (32-bit / multilib) | C compiler |
| `nasm` | Assembly |
| `ld` | Linker |
| `grub-mkrescue` or `grub2-mkrescue` | ISO creation |
| `xorriso` | ISO backend |
| `qemu-system-i386` | Emulation |

### Fedora

```bash
sudo dnf install gcc nasm make grub2-tools xorriso qemu-system-x86 glibc-devel.i686 libgcc.i686
```

### Debian/Ubuntu

```bash
sudo apt-get install gcc-multilib nasm make grub-pc-bin xorriso qemu-system-x86
```

## Build

```bash
chmod +x build.sh
./build.sh
```

Or with Make directly:

```bash
make iso
```

Output: `build/violetos.iso`

## Run in QEMU

```bash
make run
```

With persistent disk (filesystem saved across sessions):

```bash
make disk-image
make run-disk
```

## Boot from USB (Real Hardware)

1. Build the ISO: `make iso`
2. Insert a USB drive (⚠️ this erases the drive):

```bash
sudo dd if=build/violetos.iso of=/dev/sdX bs=4M status=progress conv=fsync
```

Replace `/dev/sdX` with your USB device (e.g. `/dev/sdb`). **Double-check the device name.**

3. Reboot, enter BIOS/UEFI boot menu, select the USB drive.
4. For UEFI-only machines, enable Legacy/CSM mode or use a BIOS-compatible boot.

## Project Structure

```
VioletOS/
├── boot/           Multiboot entry + GRUB config
├── kernel/         GDT, IDT, memory, main
├── drivers/        VGA, keyboard, timer, ATA, PCI, RTL8139, mouse
├── net/            TCP/IP + HTTP client
├── browser/        Web browser UI
├── filesystem/     VioletFS
├── gui/            Logo, main menu
├── shell/          Shell + text editor
├── ide/            Violet IDE
├── violet_lang/    Violet language interpreter
├── lib/            String/memory utilities
├── include/        Headers
├── docs/           Architecture documentation
├── build/          Build output (generated)
├── Makefile
├── linker.ld
└── README.md
```

## Main Menu

On boot, use arrow keys or `1`–`3` to select:

1. **Terminal** — interactive shell
2. **Violet IDE** — code editor for `.v` files
3. **Shutdown** — power off

## Shell Commands

| Command | Description |
|---------|-------------|
| `help` | List commands |
| `clear` | Clear screen |
| `ls` | List files |
| `pwd` | Current directory |
| `mkdir <dir>` | Create directory |
| `touch <file>` | Create file |
| `rm <file>` | Delete file |
| `cat <file>` | Print file |
| `not <file>` | Text editor (Ctrl+O save, Ctrl+Q quit) |
| `ide` | Open Violet IDE |
| `run <file.v>` | Run a Violet program |
| `fetch` | System info |
| `reboot` | Restart |
| `shutdown` | Power off |
| `echo <text>` | Print text |
| `date` | Uptime clock |
| `exit` | Return to main menu |

The terminal shows a **blinking block cursor** at the current input position while you type a command.

## Violet IDE

Open from the main menu. The IDE includes:

- **Title bar** — filename, modified indicator (`*`), keyboard shortcuts
- **Line numbers** — gutter with `1 |`, `2 |`, … on the left
- **Active line highlight** — current line shown with purple gutter and blue background
- **Software cursor** — white block showing exactly where the next character will appear
- **Scroll** — long files scroll automatically; status bar shows `Ln X/Y`
- **Command bar** — type `:` then `open`, `save`, `new`, or `run`

| Shortcut | Action |
|----------|--------|
| `Ctrl+S` | Save file |
| `Ctrl+R` | Run Violet code |
| `Ctrl+Q` | Quit (auto-saves if modified) |
| `:` | Command mode (`open dosya.v`, `save`, `new`, `run`) |
| Arrow keys | Move cursor |

## Violet Language

Files use the `.v` extension. Example:

```v
// Violet ornek
print "Merhaba VioletOS!"

set ad "Violet"
set sayi 10
print ad + " dili"

if sayi > 5
  print "Buyuk"
else
  print "Kucuk"
end

set i 0
while i < 3
  print i
  set i i + 1
end
```

### Data types

| Type | Example |
|------|---------|
| String | `"merhaba"` |
| Number | `10`, `-3` |
| Boolean | `true`, `false` |

### Commands

| Command | Description |
|---------|-------------|
| `print expr` | Print value or expression |
| `set name expr` | Assign variable |
| `input name` | Read user input (auto-detects type) |
| `if cond ... else ... end` | Conditional |
| `while cond ... end` | Loop while true |
| `loop N ... end` | Repeat N times (N can be a variable) |
| `clear` | Clear screen |
| `#` or `//` | Comment |

### Operators

- Math: `+ - * /`
- Compare: `== != > < >= <=`
- Logic: `and`, `or`, `not`
- String concat: `"a" + "b"`

Run from IDE (`Ctrl+R` or `:run`) or terminal: `run program.v`

## Keyboard (Turkish Q)

- Full Turkish Q letter mapping (ğ, ü, ş, ı, ö, ç, İ, etc.)
- Space, comma, period, and slash on standard scancodes
- Shift, Ctrl, Caps Lock, Backspace, Enter, Arrow keys

## Architecture

```
GRUB → Multiboot → kernel_main()
                      ├── gdt_init()
                      ├── vga_init()
                      ├── memory_init()
                      ├── idt_init()
                      ├── keyboard_init()
                      ├── timer_init()
                      ├── ata_init()
                      ├── fs_init()
                      └── gui_main_menu()
                            ├── shell_run()
                            └── ide_run()
```

See [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) for detailed technical documentation.

## Visual Theme

| Element | Color |
|---------|-------|
| Background | Black |
| Accent | Purple / Magenta |
| IDE active line | Blue background |
| IDE cursor | White block |
| Terminal cursor | Blinking hardware block |
| Text | Light purple / White |

## License

Educational use — VioletOS is provided as a learning resource for operating system development.

## Version

- **VioletOS:** 1.0.0
- **Kernel:** 1.0.0
