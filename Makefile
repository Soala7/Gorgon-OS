.PHONY: all
all: build-x86_64

# Find x86_64 assembly source files
x86_64_asm_source_files := $(shell find src/x86_64 -name '*.asm')

# Convert .asm files to .o object files
x86_64_asm_object_files := $(patsubst src/x86_64/%.asm,build/x86_64/%.o,$(x86_64_asm_source_files))

# Find x86_64 C source files
x86_64_c_source_files := $(shell find src/x86_64 -name '*.c')

# Convert .c files to .o object files
x86_64_c_object_files := $(patsubst src/x86_64/%.c,build/x86_64/%.o,$(x86_64_c_source_files))

# Find kernel C source files
kernel_source_files := $(shell find src/kernel -name '*.c')

# Convert kernel .c files to .o object files
kernel_object_files := $(patsubst src/kernel/%.c,build/x86_64/%.o,$(kernel_source_files))

# Store all x86_64 objects
x86_64_object_files := $(x86_64_c_object_files) $(x86_64_asm_object_files)

# Assemble each x86_64 source file
$(x86_64_asm_object_files): build/x86_64/%.o : src/x86_64/%.asm
	mkdir -p $(dir $@) && \
	nasm -f elf64 -g -F dwarf $< -o $@

# Compile each x86_64 C source file
$(x86_64_c_object_files): build/x86_64/%.o : src/x86_64/%.c
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -c -ffreestanding -g -O0 $< -o $@

# Compile each kernel C source file
$(kernel_object_files): build/x86_64/%.o : src/kernel/%.c
	mkdir -p $(dir $@) && \
	x86_64-elf-gcc -c -ffreestanding -g -O0 $< -o $@

.PHONY: build-x86_64

# Build the x86_64 kernel
build-x86_64: $(kernel_object_files) $(x86_64_object_files)
	mkdir -p dist/x86_64

	# Link all object files into the kernel
	x86_64-elf-ld -n \
		-o dist/x86_64/kernel.bin \
		-T targets/x86_64/linker.ld \
		$(kernel_object_files) \
		$(x86_64_object_files)

	# Copy kernel into the ISO
	cp dist/x86_64/kernel.bin targets/x86_64/iso/boot/kernel.bin

	# Create bootable ISO
	grub-mkrescue \
		-o dist/x86_64/gorgon.iso \
		targets/x86_64/iso