/* user.ld - F4-30: a static Linux user program. The section names and the constructor list
 * are the same as the kernels' linker scripts, so cxxrt.cc's run_global_constructors works. */
ENTRY(_start)
SECTIONS
{
    . = 0x10000 + SIZEOF_HEADERS;
    .text : { *(.text .text.*) }
    . = ALIGN(0x10000);
    .rodata : {
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }
    . = ALIGN(0x10000);
    .data : { *(.data .data.* .sdata .sdata.*) }
    .bss : { *(.bss .bss.* .sbss .sbss.* COMMON) }
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) *(.ARM.exidx*) *(.MIPS.abiflags) *(.reginfo) }
}
