/* rv.ld - BR-07 forensic lab: Lena's kernel, linked where OpenSBI jumps on both QEMU machines. */
ENTRY(_start)
SECTIONS
{
    . = 0x80200000;
    .text : { KEEP(*(.text.start)) *(.text .text.*) }
    .rodata : { *(.rodata .rodata.*) }
    .data : { *(.data .data.*) }
    .bss : { *(.bss .bss.*) *(COMMON) }
    /DISCARD/ : { *(.comment) *(.note.*) *(.eh_frame*) }
}
