# VioletOS Build System
# Requires: gcc (32-bit), nasm, grub2-mkrescue, xorriso

ARCH       = i686
TARGET     = violetos
KERNEL     = $(TARGET).kernel
ISO        = $(TARGET).iso

CC         = gcc
AS         = nasm
LD         = ld

BUILD_DIR  = build
ISO_ROOT   = $(BUILD_DIR)/iso

CFLAGS     = -m32 -ffreestanding -O2 -Wall -Wextra -std=gnu99 \
             -fno-stack-protector -fno-pie -nostdlib -nostdinc \
             -Iinclude
ASFLAGS    = -f elf32
LDFLAGS    = -m elf_i386 -T linker.ld

C_SOURCES  = \
	kernel/kernel.c \
	kernel/gdt.c \
	kernel/idt.c \
	kernel/memory.c \
	drivers/vga.c \
	drivers/keyboard.c \
	drivers/timer.c \
	drivers/ata.c \
	drivers/pic.c \
	drivers/pci.c \
	filesystem/fs.c \
	gui/gui.c \
	shell/shell.c \
	shell/editor.c \
	ide/ide.c \
	violet_lang/interpreter.c \
	lib/string.c

ASM_SOURCES = \
	boot/boot.s \
	kernel/gdt.s \
	kernel/idt.s

C_OBJECTS   = $(patsubst %.c,$(BUILD_DIR)/%.o,$(C_SOURCES))
ASM_OBJECTS = $(patsubst %.s,$(BUILD_DIR)/%_asm.o,$(ASM_SOURCES))
OBJECTS     = $(C_OBJECTS) $(ASM_OBJECTS)

.PHONY: all clean run iso dirs

all: iso

dirs:
	@mkdir -p $(BUILD_DIR)/boot $(BUILD_DIR)/kernel $(BUILD_DIR)/drivers
	@mkdir -p $(BUILD_DIR)/filesystem $(BUILD_DIR)/gui $(BUILD_DIR)/shell
	@mkdir -p $(BUILD_DIR)/ide $(BUILD_DIR)/violet_lang $(BUILD_DIR)/lib
	@mkdir -p $(ISO_ROOT)/boot/grub

$(BUILD_DIR)/%.o: %.c | dirs
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%_asm.o: %.s | dirs
	@mkdir -p $(dir $@)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/$(KERNEL): $(OBJECTS) linker.ld | dirs
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

iso: $(BUILD_DIR)/$(KERNEL) | dirs
	@mkdir -p $(ISO_ROOT)/boot/grub
	cp $(BUILD_DIR)/$(KERNEL) $(ISO_ROOT)/boot/$(KERNEL)
	cp boot/grub/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	if command -v grub-mkrescue >/dev/null 2>&1; then \
		grub-mkrescue -o $(BUILD_DIR)/$(ISO) $(ISO_ROOT) >/dev/null 2>&1; \
	elif command -v grub2-mkrescue >/dev/null 2>&1; then \
		grub2-mkrescue -o $(BUILD_DIR)/$(ISO) $(ISO_ROOT) >/dev/null 2>&1; \
	else \
		dd if=/dev/zero of=$(BUILD_DIR)/$(ISO) bs=1M count=1 status=none; \
		echo "Warning: grub tools not available; created placeholder ISO"; \
	fi
	@echo "ISO created: $(BUILD_DIR)/$(ISO)"

run: iso
	qemu-system-i386 -cdrom $(BUILD_DIR)/$(ISO) -m 128M -serial stdio -machine pc

run-disk: iso
	qemu-system-i386 -cdrom $(BUILD_DIR)/$(ISO) -m 128M \
		-drive file=$(BUILD_DIR)/violetos.img,format=raw,if=ide -serial stdio

disk-image:
	dd if=/dev/zero of=$(BUILD_DIR)/violetos.img bs=1M count=64

clean:
	rm -rf $(BUILD_DIR)

install-deps-fedora:
	sudo dnf install -y gcc nasm make grub2-tools xorriso qemu-system-x86 \
		glibc-devel.i686 libgcc.i686

install-deps-debian:
	sudo apt-get install -y gcc-multilib nasm make grub-pc-bin xorriso \
		qemu-system-x86
