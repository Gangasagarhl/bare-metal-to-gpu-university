/* kernel.ld - link the OS301 kernel stub in the higher half, one PT_LOAD segment per permission
   set, each starting on its own 4 KiB page so that the loader can map them separately. */
ENTRY(kernel_main)

PHDRS
{
    text   PT_LOAD FLAGS(5);   /* R + X */
    rodata PT_LOAD FLAGS(4);   /* R     */
    data   PT_LOAD FLAGS(6);   /* R + W */
}

SECTIONS
{
    . = 0xffffffff80000000;
    .text : { *(.text .text.*) } :text
    . = ALIGN(4096);
    .rodata : { *(.rodata .rodata.*) } :rodata
    . = ALIGN(4096);
    .data : { *(.data .data.*) } :data
    .bss : { *(.bss .bss.* COMMON) } :data
    /DISCARD/ : { *(.comment) *(.eh_frame*) *(.note*) }
}
