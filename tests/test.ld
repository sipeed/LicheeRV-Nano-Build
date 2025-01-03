MEMORY {
  kernel_space (rwx) : ORIGIN = 0x80200000, LENGTH = 128K
}

SECTIONS {
    . = ORIGIN(kernel_space);

    .header ALIGN(4) : {
        KEEP(*(.header))
    } > kernel_space

    .text.kernel ALIGN(4) : {
        *(.text.kernel)
    } > kernel_space

    .rodata ALIGN(4) : {
        *(.rodata)
    } > kernel_space

    .data ALIGN(4) : {
        *(.data)
    } > kernel_space

    .bss ALIGN(4) : {
        *(COMMON)
        *(.bss)
    } > kernel_space

    _end = .;
    _. = ASSERT((_end <= (ORIGIN(kernel_space) + LENGTH(kernel_space))), "Binary exceeds available memory");
}
