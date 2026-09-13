.PHONY: all build-x86_64 clean run

all: build-x86_64

x86_64_asm_source_files := $(shell find src/x86_64 -name '*.asm')
x86_64_asm_object_files := $(patsubst src/x86_64/%.asm,build/x86_64/%.o,$(x86_64_asm_source_files))

x86_64_c_source_files := $(shell find src/x86_64 -name '*.c')
x86_64_c_object_files := $(patsubst src/x86_64/%.c,build/x86_64/%.o,$(x86_64_c_source_files))

kernel_source_files := $(shell find src/kernel -name '*.c')
kernel_object_files := $(patsubst src/kernel/%.c,build/x86_64/%.o,$(kernel_source_files))

x86_64_object_files := $(x86_64_c_object_files) $(x86_64_asm_object_files)

CFLAGS := -ffreestanding -g -O0 -Wall \
          -mno-red-zone \
          -fno-stack-protector \
          -fno-pic \
          -fno-pie \
          -mno-sse

ASFLAGS := -f elf64 -g -F dwarf

LDFLAGS := -n

$(x86_64_asm_object_files): build/x86_64/%.o : src/x86_64/%.asm
	mkdir -p $(dir $@)
	nasm $(ASFLAGS) $< -o $@

$(x86_64_c_object_files): build/x86_64/%.o : src/x86_64/%.c
	mkdir -p $(dir $@)
	x86_64-elf-gcc -c $(CFLAGS) $< -o $@

$(kernel_object_files): build/x86_64/%.o : src/kernel/%.c
	mkdir -p $(dir $@)
	x86_64-elf-gcc -c $(CFLAGS) $< -o $@

build-x86_64: $(kernel_object_files) $(x86_64_object_files)
	mkdir -p dist/x86_64

	x86_64-elf-ld $(LDFLAGS) \
		-o dist/x86_64/kernel.bin \
		-T targets/x86_64/linker.ld \
		$(kernel_object_files) \
		$(x86_64_object_files)

	mkdir -p targets/x86_64/iso/boot
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin

	grub-mkrescue \
		-o dist/x86_64/gorgon.iso \
		targets/x86_64/iso

clean:
	rm -rf build/x86_64 dist/x86_64

run: build-x86_64
	qemu-system-x86_64 \
		-cdrom dist/x86_64/gorgon.iso \
		-serial stdio \
		-no-reboot \
		-no-shutdown \
		-d int,cpu_reset