/* rv_virt.ld - linker script for the RISC-V comparison (QEMU virt).
 * RAM origin from QEMU's memory tree (riscv_virt_board.ram at 0x80000000). */
MEMORY
{
    RAM (rwx) : ORIGIN = 0x80000000, LENGTH = 1M
}
ENTRY(_start)
SECTIONS
{
    .text : { *(.text.start) *(.text .text.*) *(.rodata .rodata.*) } > RAM
    .data : { *(.data .data.* .sdata .sdata.*) } > RAM
    .bss (NOLOAD) : { . = ALIGN(8); __bss_start = .; *(.bss .bss.* .sbss .sbss.* COMMON); . = ALIGN(8); __bss_end = .; } > RAM
    __stack_top = ORIGIN(RAM) + LENGTH(RAM);
    /DISCARD/ : { *(.comment) *(.riscv.attributes) *(.note*) *(.eh_frame*) }
}
