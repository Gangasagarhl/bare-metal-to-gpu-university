/* bmc_hello.ld - place the program in the AST2600 model's DRAM (QEMU memory map: "ram"
   aliased at 0x80000000); the generic loader starts the first CPU at the ELF entry point. */
ENTRY(start)
SECTIONS
{
    . = 0x80000000;
    .text : { *(.text*) }
    .rodata : { *(.rodata*) }
    .data : { *(.data*) }
    .bss : { *(.bss*) }
}
