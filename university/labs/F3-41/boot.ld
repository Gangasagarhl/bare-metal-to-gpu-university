/* boot.ld - linker script of the boot loader (F3-41): the first 16 KiB of code memory. */
MEMORY
{
    FLASH (rx)  : ORIGIN = 0x00000000, LENGTH = 16K
    RAM   (rwx) : ORIGIN = 0x20000000, LENGTH = 64K
}
ENTRY(Reset_Handler)
SECTIONS
{
    .text : { KEEP(*(.vectors)) *(.text .text.*) *(.rodata .rodata.*) . = ALIGN(4);
              __init_array_start = .; KEEP(*(.init_array .init_array.*)) __init_array_end = .;
              __text_end = .; } > FLASH
    .data : { . = ALIGN(4); __data_start = .; *(.data .data.*) . = ALIGN(4); __data_end = .; } > RAM AT > FLASH
    __data_load = LOADADDR(.data);
    .bss (NOLOAD) : { . = ALIGN(4); __bss_start = .; *(.bss .bss.* COMMON) . = ALIGN(4); __bss_end = .; } > RAM
    __stack_top = ORIGIN(RAM) + LENGTH(RAM);
    /DISCARD/ : { *(.ARM.exidx*) *(.comment) }
}
