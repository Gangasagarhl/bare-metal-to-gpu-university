/* boot.S - OS303 teaching kernel: Multiboot (version 1) entry, 32-bit to 64-bit switch.
 * QEMU's -kernel option loads a Multiboot kernel and enters _start in 32-bit protected
 * mode with paging off, EAX = Multiboot magic and EBX = address of the Multiboot
 * information structure (Multiboot Specification 0.6.96, "Machine state"; title only,
 * not opened in this build). We build page tables that identity-map the first 4 GiB
 * with 2 MiB pages, enable PAE + long mode + paging, and jump to 64-bit C++ (kmain).
 * Bit numbers below follow the Intel SDM Vol. 3 chapters "Paging" and "IA-32e mode"
 * (not opened in this build: see the chapter's unverified box); the QEMU run proves
 * that they work on QEMU 8.2.2's CPU model, nothing more. */
    .set MB_MAGIC, 0x1BADB002
    .set MB_FLAGS, 0x00000003           /* bit 0: page-align modules, bit 1: memory info */

    .section .multiboot, "a"
    .align 4
    .long MB_MAGIC
    .long MB_FLAGS
    .long -(MB_MAGIC + MB_FLAGS)

    .section .data
    .global mb_magic, mb_info
mb_magic: .long 0
mb_info:  .long 0
    .align 16
gdt64:                                  /* boot GDT; cpu.cc installs the real one */
    .quad 0
    .quad 0x00AF9A000000FFFF            /* 0x08 kernel code, 64-bit (L=1) */
    .quad 0x00CF92000000FFFF            /* 0x10 kernel data */
gdt64_end:
gdt64_ptr:
    .word gdt64_end - gdt64 - 1
    .quad gdt64

    .section .bss
    .align 4096
    .global boot_pml4, boot_pdpt
boot_pml4:  .skip 4096
boot_pdpt:  .skip 4096
boot_pd:    .skip 4096 * 4              /* four page directories: 4 x 1 GiB */
boot_stack: .skip 16384
boot_stack_top:

    .section .text.boot, "ax"
    .code32
    .global _start
_start:
    cli
    mov %eax, mb_magic
    mov %ebx, mb_info
    /* zero .bss (page tables and the boot stack live there) */
    mov $__bss_start, %edi
    mov $__bss_end, %ecx
    sub %edi, %ecx
    xor %eax, %eax
    rep stosb
    mov $boot_stack_top, %esp

    /* PML4[0] -> PDPT, PDPT[0..3] -> the four page directories (present + writable) */
    mov $boot_pdpt, %eax
    or $0x3, %eax
    mov %eax, boot_pml4
    mov $boot_pd, %eax
    or $0x3, %eax
    xor %ecx, %ecx
1:  mov %eax, boot_pdpt(,%ecx,8)
    add $4096, %eax
    inc %ecx
    cmp $4, %ecx
    jne 1b

    /* 2048 PD entries, entry i maps 2 MiB at i << 21: present, writable, PS (bit 7).
       The top 1 GiB (local APIC, I/O APIC, other MMIO) also gets PCD|PWT (uncached). */
    xor %ecx, %ecx
2:  mov %ecx, %eax
    shl $21, %eax
    or $0x83, %eax
    cmp $1536, %ecx
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
    or $0x20, %eax                      /* CR4.PAE */
    mov %eax, %cr4
    mov $0xC0000080, %ecx               /* IA32_EFER */
    rdmsr
    or $0x100, %eax                     /* EFER.LME */
    wrmsr
    mov %cr0, %eax
    or $0x80000001, %eax                /* CR0.PG | CR0.PE */
    mov %eax, %cr0
    lgdt gdt64_ptr
    ljmp $0x08, $long_mode

    .code64
long_mode:
    mov $0x10, %ax
    mov %ax, %ds
    mov %ax, %es
    mov %ax, %ss
    xor %ax, %ax
    mov %ax, %fs
    mov %ax, %gs
    mov $boot_stack_top, %rsp
    call kmain                          /* never returns */
4:  hlt
    jmp 4b
