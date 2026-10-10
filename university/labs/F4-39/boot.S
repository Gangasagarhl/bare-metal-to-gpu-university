/* boot.S - DR404 lab kernel entry: Multiboot (version 1), 32-bit to 64-bit switch.
 * QEMU's -kernel loader (and the F4-41 hypervisor, which imitates it) enters _start in
 * 32-bit protected mode with paging off, EAX = 0x2BADB002 and EBX = the address of the
 * Multiboot information structure (Multiboot Specification, "Machine state": title only,
 * not opened in this build). We identity-map the first 4 GiB with 2 MiB pages, switch on
 * PAE, long mode and paging, and call kmain(magic, info) in 64-bit C++.
 * Second entry: a loader that has already switched to long mode (the F4-41 hypervisor)
 * may jump to long_mode after writing boot_magic and boot_info, with a GDT like gdt64.
 * Bit numbers follow the Intel SDM Vol. 3 chapters on paging and IA-32e mode (not opened
 * in this build; the chapter's unverified box says so). The QEMU runs prove they work
 * on QEMU 8.2.2's CPU model and on the F4-41 hypervisor, nothing more. */
    .set MB_MAGIC, 0x1BADB002
    .set MB_FLAGS, 0x00000003          /* page-align modules; give us memory info */

    .section .multiboot, "a"
    .align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long -(MB_MAGIC + MB_FLAGS)

    .section .data
    .align 16
gdt64:
    .quad 0
    .quad 0x00AF9A000000FFFF           /* 0x08: 64-bit kernel code (L=1) */
    .quad 0x00CF92000000FFFF           /* 0x10: kernel data */
gdt64_end:
    .global gdt64_ptr
gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64
    .global boot_magic, boot_info      /* global: the F4-41 hypervisor fills them in */
boot_magic: .long 0
boot_info:  .long 0

    .section .bss
    .align 4096
boot_pml4:  .skip 4096
boot_pdpt:  .skip 4096
boot_pd:    .skip 4096 * 4             /* 4 page directories x 512 entries x 2 MiB */
boot_stack: .skip 32768
boot_stack_top:

    .section .text.boot, "ax"
    .code32
    .global _start
_start:
    cli
    mov %eax, boot_magic
    mov %ebx, boot_info
    mov $__bss_start, %edi             /* zero .bss: page tables, stack, globals */
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb
    mov $boot_stack_top, %esp

    mov $boot_pdpt, %eax               /* PML4[0] -> PDPT (present | writable) */
    or $0x3, %eax
    mov %eax, boot_pml4
    mov $boot_pd, %eax                 /* PDPT[0..3] -> the four page directories */
    or $0x3, %eax
    xor %ecx, %ecx
1:  mov %eax, boot_pdpt(,%ecx,8)
    add $4096, %eax
    inc %ecx
    cmp $4, %ecx
    jne 1b
    xor %ecx, %ecx                     /* PD entry i maps 2 MiB at i << 21 */
2:  mov %ecx, %eax
    shl $21, %eax
    or $0x83, %eax                     /* present | writable | PS (2 MiB page) */
    cmp $1536, %ecx                    /* the top GiB holds the APICs: uncached */
    jb 3f
    or $0x18, %eax
3:  mov %eax, boot_pd(,%ecx,8)
    movl $0, boot_pd+4(,%ecx,8)
    inc %ecx
    cmp $2048, %ecx
    jne 2b

    mov $boot_pml4, %eax
    mov %eax, %cr3
    mov %cr4, %eax
    or $0x20, %eax                     /* CR4.PAE */
    mov %eax, %cr4
    mov $0xC0000080, %ecx              /* EFER: read-modify-write keeps other bits */
    rdmsr
    or $0x100, %eax                    /* EFER.LME */
    wrmsr
    mov %cr0, %eax
    or $0x80000001, %eax               /* CR0.PG | CR0.PE */
    mov %eax, %cr0
    lgdt gdt64_ptr
    ljmp $0x08, $long_mode

    .code64
    .global long_mode                  /* second entry point: already in long mode (F4-41) */
long_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    xor %ax, %ax
    mov %ax, %fs
    mov %ax, %gs
    mov $boot_stack_top, %rsp
    mov boot_magic(%rip), %edi
    mov boot_info(%rip), %esi
    call kmain                         /* never returns */
4:  cli
    hlt
    jmp 4b
