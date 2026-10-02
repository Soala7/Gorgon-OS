ENTRY(start)

KERNEL_VMA = 0xFFFFFFFF80000000;

SECTIONS
{
    . = 1M;

    kernel_load_start = .;

    /*
     * Low bootstrap code/data.
     * These must be accessible before the higher-half transition.
     */
    .boot ALIGN(4K) :
    {
        KEEP(*(.multiboot_header))
    }

    .boot.text ALIGN(4K) :
    {
        *(.boot.text)
    }

    .boot.data ALIGN(4K) :
    {
        *(.boot.data)
    }
    .boot.bss ALIGN(4K) (NOLOAD) :
    {
        *(.boot.bss)
    }

    /*
     * Kernel proper.
     *
     * VMA = higher half
     * LMA = low physical memory
     */
    . += KERNEL_VMA;

    kernel_start = .;
    kernel_start_phys = kernel_start - KERNEL_VMA;

    .text ALIGN(4K) : AT(ADDR(.text) - KERNEL_VMA)
    {
        *(.text)
    }

    .rodata ALIGN(4K) : AT(ADDR(.rodata) - KERNEL_VMA)
    {
        *(.rodata)
    }

    .eh_frame ALIGN(4K) : AT(ADDR(.eh_frame) - KERNEL_VMA)
    {
        *(.eh_frame)
    }

    .data ALIGN(4K) : AT(ADDR(.data) - KERNEL_VMA)
    {
        *(.data)
    }

    .bss ALIGN(4K) (NOLOAD) : AT(ADDR(.bss) - KERNEL_VMA)
    {
        *(COMMON)
        *(.bss)
    }

    . = ALIGN(4K);

    kernel_end = .;
    kernel_end_phys = kernel_end - KERNEL_VMA;
}
