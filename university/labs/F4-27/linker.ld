/* linker.ld - F4-27: the AArch64 kernel for a Raspberry Pi (and QEMU raspi3b). Pi RAM starts
 * at address 0 and the firmware loads the kernel at 0x80000 (RAM base + the header text_offset;
 * the load address on a real Pi is untested here, see the chapter). Only the base below differs
 * from F4-24/linker.ld. */
ENTRY(_start)

SECTIONS
{
    . = 0x80000;
    __image_start = .;
    .text : {
        KEEP(*(.head.text))
        *(.text .text.*)
    }
    .rodata ALIGN(16) : {
        *(.rodata .rodata.*)
        . = ALIGN(8);
        __init_array_start = .;
        KEEP(*(SORT_BY_INIT_PRIORITY(.init_array.*)))
        KEEP(*(.init_array))
        __init_array_end = .;
    }
    .data ALIGN(16) : {
        *(.data .data.*)
    }
    .bss ALIGN(16) : {
        __bss_start = .;
        *(COMMON)
        *(.bss .bss.*)
        . = ALIGN(16);
        __bss_end = .;
    }
    . = ALIGN(4096);
    . += 0x4000 * 8;              /* boot stack 16 KiB per CPU, room for 8 CPUs (F4-26) */
    __stack_top = .;
    __image_end = .;
    __image_size = __image_end - __image_start;
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
