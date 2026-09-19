# ============================================================
# Gorgon OS — x86-64 Build System
# ============================================================

# ------------------------------------------------------------
# Toolchain
# ------------------------------------------------------------

CC      := x86_64-elf-gcc
LD      := x86_64-elf-ld
NASM    := nasm
GRUB    := grub-mkrescue

# ------------------------------------------------------------
# Flags
# ------------------------------------------------------------

CFLAGS  := -ffreestanding \
           -g \
           -O0 \
           -mno-red-zone \
           -fno-stack-protector \
           -fno-pic \
           -mno-sse \
           -Wall \
           -I src \
           -I src/kernel \
           -I src/x86_64

LDFLAGS := -T targets/x86_64/linker.ld

NASMFLAGS := -f elf64 -g -F dwarf

# ------------------------------------------------------------
# Directories
# ------------------------------------------------------------

BUILD := build/x86_64
DIST  := dist/x86_64
ISO   := $(DIST)/iso

# ------------------------------------------------------------
# C Sources
# ------------------------------------------------------------

C_SOURCES := \
	src/kernel/main.c \
	src/kernel/serial.c \
	src/kernel/task.c \
	src/x86_64/cpu/pic.c \
	src/x86_64/cpu/timer.c \
	src/x86_64/cpu/tss.c \
	src/x86_64/memory/pmm.c

# ------------------------------------------------------------
# Assembly Sources
# ------------------------------------------------------------

ASM_SOURCES := \
	src/x86_64/boot/header.asm \
	src/x86_64/boot/main.asm \
	src/x86_64/boot/main64.asm \
	src/x86_64/cpu/gdt.asm \
	src/x86_64/cpu/idt.asm \
	src/x86_64/cpu/pit.asm \
	src/x86_64/cpu/context_switch.asm

# ------------------------------------------------------------
# Object Files
# ------------------------------------------------------------

C_OBJECTS := \
	$(BUILD)/main.o \
	$(BUILD)/serial.o \
	$(BUILD)/task.o \
	$(BUILD)/cpu/pic.o \
	$(BUILD)/cpu/timer.o \
	$(BUILD)/cpu/tss.o \
	$(BUILD)/memory/pmm.o

ASM_OBJECTS := \
	$(BUILD)/boot/header.o \
	$(BUILD)/boot/main.o \
	$(BUILD)/boot/main64.o \
	$(BUILD)/cpu/gdt.o \
	$(BUILD)/cpu/idt.o \
	$(BUILD)/cpu/pit.o \
	$(BUILD)/cpu/context_switch.o

OBJECTS := $(C_OBJECTS) $(ASM_OBJECTS)

# ------------------------------------------------------------
# Default Target
# ------------------------------------------------------------

.PHONY: all
all: $(DIST)/gorgon.iso

# ------------------------------------------------------------
# Compile C — root kernel files
# ------------------------------------------------------------

$(BUILD)/main.o: src/kernel/main.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/serial.o: src/kernel/serial.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/task.o: src/kernel/task.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

# ------------------------------------------------------------
# Compile C — CPU
# ------------------------------------------------------------

$(BUILD)/cpu/pic.o: src/x86_64/cpu/pic.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/cpu/timer.o: src/x86_64/cpu/timer.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

$(BUILD)/cpu/tss.o: src/x86_64/cpu/tss.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

# ------------------------------------------------------------
# Compile C — Memory
# ------------------------------------------------------------

$(BUILD)/memory/pmm.o: src/x86_64/memory/pmm.c
	mkdir -p $(dir $@)
	$(CC) -c $(CFLAGS) $< -o $@

# ------------------------------------------------------------
# Assemble
# ------------------------------------------------------------

$(BUILD)/boot/header.o: src/x86_64/boot/header.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/boot/main.o: src/x86_64/boot/main.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/boot/main64.o: src/x86_64/boot/main64.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/cpu/gdt.o: src/x86_64/cpu/gdt.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/cpu/idt.o: src/x86_64/cpu/idt.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/cpu/pit.o: src/x86_64/cpu/pit.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

$(BUILD)/cpu/context_switch.o: src/x86_64/cpu/context_switch.asm
	mkdir -p $(dir $@)
	$(NASM) $(NASMFLAGS) $< -o $@

# ------------------------------------------------------------
# Link Kernel
# ------------------------------------------------------------

$(DIST)/kernel.bin: $(OBJECTS)
	mkdir -p $(DIST)
	$(LD) $(LDFLAGS) -o $@ $(OBJECTS)

# ------------------------------------------------------------
# Build ISO
# ------------------------------------------------------------

$(DIST)/gorgon.iso: $(DIST)/kernel.bin targets/x86_64/iso/boot/grub/grub.cfg
	rm -rf $(ISO)
	mkdir -p $(ISO)/boot/grub
	cp $(DIST)/kernel.bin $(ISO)/boot/kernel.bin
	cp targets/x86_64/iso/boot/grub/grub.cfg $(ISO)/boot/grub/grub.cfg
	$(GRUB) -o $@ $(ISO)

# ------------------------------------------------------------
# Run
# ------------------------------------------------------------

.PHONY: run
run: $(DIST)/gorgon.iso
	qemu-system-x86_64 \
		-cdrom $(DIST)/gorgon.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown

# ------------------------------------------------------------
# Debug (GDB)
# ------------------------------------------------------------
# -s starts QEMU's GDB stub on localhost:1234.
# -S pauses the CPU before the first instruction.
#
# Usage:
#   Terminal 1:
#       make debug
#
#   Terminal 2:
#       gdb dist/x86_64/kernel.bin
#
#   In GDB:
#       target remote localhost:1234
#       break kernel_main
#       continue

.PHONY: debug
debug: $(DIST)/gorgon.iso
	qemu-system-x86_64 \
		-cdrom $(DIST)/gorgon.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-d int \
		-s -S

# ------------------------------------------------------------
# Clean
# ------------------------------------------------------------

.PHONY: clean
clean:
	rm -rf build
	rm -rf dist/x86_64/kernel.bin
	rm -rf dist/x86_64/gorgon.iso
	rm -rf dist/x86_64/iso

# ------------------------------------------------------------
# Help
# ------------------------------------------------------------

.PHONY: help
help:
	@echo "Gorgon OS build targets:"
	@echo "  make        Build kernel and ISO"
	@echo "  make run    Build and run Gorgon in QEMU"
	@echo "  make debug  Build and run with GDB stub (see comment above debug target)"