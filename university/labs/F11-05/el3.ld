/* el3.ld - F11-05: the monitor's code and constants in the secure flash at address 0,
   its stack and secret in secure RAM (0x0e000000, 16 MiB on QEMU virt with secure=on). */
ENTRY(_start)
SECTIONS
{
    . = 0x0;
    .text : { *(.text.boot) *(.text*) }
    .rodata : { *(.rodata*) *(.data*) }
    . = 0x0e000000;
    .secure_bss (NOLOAD) : { *(.secure_bss) *(.bss*) *(COMMON) . = ALIGN(16); }
    . = 0x0e010000;
    __el3_stack_top = .;
    /DISCARD/ : { *(.comment) }
}
