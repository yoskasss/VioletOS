# VioletOS Documentation

## Overview

VioletOS is an educational operating system targeting the x86 32-bit architecture. It demonstrates core OS concepts including bootstrapping, segmentation, interrupts, device drivers, memory management, filesystem design, and user-facing applications running entirely in kernel mode.

Recent additions include the **Violet IDE** (line-numbered editor with software cursor) and the **Violet** interpreted programming language for `.v` scripts.

## Memory Layout

| Region | Address | Description |
|--------|---------|-------------|
| VGA Text | 0xB8000 | 80×25 text buffer |
| VGA Graphics | 0xA0000 | Mode 13h framebuffer (legacy, unused in current UI) |
| Kernel load | 0x100000 (1 MB) | Multiboot load address |
| Kernel heap | 0x1000000 | 4 MB bump/linked-list heap |

## Boot Process

1. **GRUB** loads the kernel using the Multiboot specification.
2. **`boot/boot.s`** sets up the stack and calls `kernel_main()`.
3. **`kernel_main()`** initializes subsystems in order: GDT → VGA → Memory → IDT → Keyboard → Timer → ATA → Filesystem.
4. The ASCII logo is displayed, then the main menu appears.

## GDT

Five segments are defined:
- Null descriptor
- Kernel code (0x08)
- Kernel data (0x10)
- User code (0x18)
- User data (0x20)

## IDT

- ISR 0–31: CPU exception handlers
- IRQ 0 (ISR 32): Timer interrupt → `timer_handler()`
- IRQ 1 (ISR 33): Keyboard interrupt → `keyboard_handler()`
- ISR 8 (double fault): assembly halt loop — does not call C on corrupt stack
- Unhandled IRQs 32–47: EOI sent via `irq_dispatch()` to prevent interrupt storms

## Drivers

### VGA
- **Text mode:** 80×25, custom purple-on-black color scheme
- **`vga_putchar_at()`** — direct cell writes for UI rendering (IDE)
- **`vga_show_hw_cursor()` / `vga_hide_hw_cursor()`** — CRTC block cursor control
- **Mode 13h:** 320×200×256 framebuffer

### Keyboard
- PS/2 scancode set 1
- Turkish Q layout with CP857 character codes
- Shift, Ctrl, Caps Lock
- Arrow keys encoded as `KEY_UP` (0xF0) … `KEY_RIGHT` (0xF3)
- Enter delivered as `KEY_ENTER` (0x0A)

### PIC
- IRQ 0–15 remapped to INT 32–47
- **Only timer (IRQ0) and keyboard (IRQ1) are unmasked at boot**
- `pic_irq_enable()` / `pic_irq_disable()` for dynamic IRQ control
- Unhandled IRQs receive EOI via `irq_dispatch()` to prevent double faults

### Timer
- PIT channel 0 at 100 Hz
- Used for uptime, delays

### ATA
- Primary master PIO mode
- Sectors 2048+ used for VioletFS persistence

## VioletFS

A flat filesystem stored in the superblock:

- Up to 64 entries
- Max filename: 32 bytes
- Max file content: 4096 bytes per file
- Types: file, directory
- Persisted to disk LBA 2048 when ATA is available

## GUI

Keyboard-only main menu (`gui/gui.c`):

| Option | Action |
|--------|--------|
| Terminal | `shell_run()` |
| Violet IDE | `ide_run()` |
| Shutdown | `sys_shutdown()` |

Boot logo ASCII art displays **VioletOS**.

## Shell

The shell reads input line-by-line, tokenizes into `argv`, and dispatches to command handlers.

**Cursor:** `vga_show_hw_cursor()` is enabled during the shell session. After each prompt, the hardware block cursor is positioned at `prompt_col + input_length` so the user always sees where the next character will appear. Backspace clears the cell and moves the cursor back.

Notable commands:
- `ide` — launches Violet IDE
- `not <file>` — launches the legacy full-screen text editor
- `run <file.v>` — executes a Violet program

## Violet IDE

Located in `ide/ide.c`. Full-screen editor for `.v` source files.

### Layout (80×25 text)

| Row(s) | Content |
|--------|---------|
| 0 | Title bar (magenta) — filename, shortcuts |
| 1 | Horizontal rule |
| 2–19 | Editor — line gutter + code area |
| 20 | Horizontal rule |
| 21 | Command hint (`: open | save | new | run`) |
| 22 | Status bar — messages, line counter |

### Line gutter

Each editor row shows a right-aligned line number and a `|` separator:

```
  1 | print "hello"
  2 | set a 10
```

The active line uses a purple gutter and blue code background.

### Cursor

The IDE uses a **software cursor**: the character under the insertion point is rendered as white-on-black (inverted block). The hardware cursor is hidden to avoid conflicts with full-screen redraws.

### Scrolling

When `ide_line_count > 18`, `ide_scroll` tracks the first visible line. The status bar shows `Ln current/total`.

### Commands

Typed after `:` on the command row, or via shortcuts:

| Input | Action |
|-------|--------|
| `Ctrl+S` | Save to VioletFS |
| `Ctrl+R` | Run buffer through interpreter |
| `Ctrl+Q` | Quit (save if dirty) |
| `:open file.v` | Load file |
| `:save` | Save |
| `:new` | New buffer |
| `:run` | Execute |

## Violet Language

Interpreter: `violet_lang/interpreter.c`

Line-based execution with block nesting via `end`. Supports:

- Types: String, Number, Boolean
- Statements: `print`, `set`, `input`, `clear`
- Control flow: `if` / `else` / `end`, `while` / `end`, `loop` / `end`
- Expressions: arithmetic (`+ - * /`), string concat (`+`), comparisons, `and` / `or` / `not`
- Comments: `#` or `//`

Errors report line number and source text.



## Legacy Text Editor

`shell/editor.c` — simpler editor opened by the `not` shell command. Uses arrow keys; Ctrl+O save, Ctrl+Q quit.

## Building for Real Hardware

The ISO produced by `grub-mkrescue` is hybrid-bootable. For USB:

```bash
sudo dd if=build/violetos.iso of=/dev/sdX bs=4M status=progress conv=fsync
```

Ensure the target machine supports Legacy BIOS boot from USB.

## Troubleshooting

| Issue | Solution |
|-------|----------|
| `gcc: cannot find -lgcc` with `-m32` | Install `glibc-devel.i686` / `gcc-multilib` |
| `grub-mkrescue: not found` | Install `grub2-tools` or `grub-pc-bin` |
| Blank screen in QEMU | Try `-vga std` flag |
| Keyboard not working | Click inside QEMU window to capture input |
| Filesystem not persisting | Use `make run-disk` with a disk image |
| Kernel panic on any action | Fixed: PIC masks all IRQs except timer+keyboard; EOI on spurious IRQs |
| Cursor not visible in terminal | Ensure `vga_show_hw_cursor()` is active in shell |

## Future Enhancements

- RTC real-time clock for `date` command
- Scrolling terminal output buffer
- Nested directories in VioletFS
- Violet language: functions, arrays
- Syntax highlighting in IDE
- Sound via PC speaker
- Multitasking
