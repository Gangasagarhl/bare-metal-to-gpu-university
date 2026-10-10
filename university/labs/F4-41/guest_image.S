/* guest_image.S - the F4-39 kernel (64-bit ELF) embedded in the hypervisor's image.
 * build: the run script assembles this with -DGUEST_ELF='"<path>"'. */
    .section .rodata
    .balign 16
    .global guest_image_start, guest_image_end
guest_image_start:
    .incbin GUEST_ELF
guest_image_end:
