/* boot.S - F3-18: Multiboot (version 1) entry, switch to 64-bit long mode, jump to the
 * higher-half C++ kernel. QEMU's built-in Multiboot loader (-kernel) loads the flat image at
 * physical 0x100000 and jumps to _start in 32-bit protected mode with paging off,
 * EAX = 0x2BADB002 and EBX = physical address of the Multiboot information structure. */
        .set MB_MAGIC,  0x1BADB002
        .set MB_FLAGS,  0x00010002          /* bit 1: memory info; bit 16: address fields valid */

        .section .multiboot, "a"
        .balign 4
mb_header:
        .long MB_MAGIC
        .long MB_FLAGS
        .long -(MB_MAGIC + MB_FLAGS)        /* checksum: the three fields sum to 0 */
        .long mb_header                     /* header_addr (physical) */
        .long __load_start                  /* load_addr */
        .long __load_end_phys               /* load_end_addr: end of the file image */
        .long __bss_end_phys                /* bss_end_addr */
        .long _start                        /* entry_addr */

        .section .boot.text, "ax"
        .code32
        .globl _start
_start:
        cli
        mov %eax, %edi                      /* keep magic and info pointer for kmain */
        mov %ebx, %esi
        /* Boot page tables, 2 MiB pages (F3-21 replaces them with the kernel's own):
         *   PML4[0]   -> boot_pdpt_dm : identity map of physical 0..8 GiB (only for this stub)
         *   PML4[256] -> boot_pdpt_dm : direct map, virtual 0xffff800000000000 + phys
         *   PML4[511] -> boot_pdpt_k  : PDPT[510] -> boot_pd[0], so virtual
         *               0xffffffff80000000 + phys maps physical 0..1 GiB (the kernel image) */
        mov $boot_pdpt_dm, %eax
        or  $0x3, %eax                      /* present | writable */
        mov %eax, boot_pml4
        mov %eax, boot_pml4 + 256*8
        mov $boot_pdpt_k, %eax
        or  $0x3, %eax
        mov %eax, boot_pml4 + 511*8
        xor %ecx, %ecx                      /* 8 PDPT entries -> the 8 page directories */
1:      mov %ecx, %eax
        shl $12, %eax
        add $boot_pd, %eax
        or  $0x3, %eax
        mov %eax, boot_pdpt_dm(,%ecx,8)
        inc %ecx
        cmp $8, %ecx
        jne 1b
        mov $boot_pd, %eax
        or  $0x3, %eax
        mov %eax, boot_pdpt_k + 510*8
        xor %ecx, %ecx                      /* 4096 PD entries: physical 0 .. 8 GiB */
2:      mov %ecx, %eax
        shl $21, %eax                       /* low 32 bits of ecx * 2 MiB */
        or  $0x83, %eax                     /* present | writable | page size (2 MiB) */
        mov %eax, boot_pd(,%ecx,8)
        mov %ecx, %eax
        shr $11, %eax                       /* high 32 bits of ecx * 2 MiB */
        mov %eax, boot_pd + 4(,%ecx,8)
        inc %ecx
        cmp $4096, %ecx
        jne 2b
        mov $boot_pml4, %eax
        mov %eax, %cr3
        mov %cr4, %eax
        or  $(1 << 5), %eax                 /* CR4.PAE */
        mov %eax, %cr4
        mov $0xC0000080, %ecx               /* IA32_EFER */
        rdmsr
        or  $(1 << 8), %eax                 /* EFER.LME: long mode enable */
        wrmsr
        mov %cr0, %eax
        or  $((1 << 31) | 1), %eax          /* CR0.PG | CR0.PE */
        mov %eax, %cr0
        lgdt boot_gdt_ptr
        ljmp $0x08, $long_mode_low

        .code64
long_mode_low:
        movabs $long_mode_high, %rax        /* continue at the higher-half address */
        jmp *%rax

        .section .boot.data, "aw"
        .balign 4096
        .globl boot_pml4, boot_pd
boot_pml4:    .fill 512, 8, 0
boot_pdpt_dm: .fill 512, 8, 0
boot_pdpt_k:  .fill 512, 8, 0
boot_pd:      .fill 4096, 8, 0              /* 8 page directories, one after the other */
boot_gdt:
        .quad 0                             /* null descriptor */
        .quad 0x00AF9A000000FFFF            /* 0x08: 64-bit code, ring 0 */
        .quad 0x00CF92000000FFFF            /* 0x10: data, ring 0 */
boot_gdt_end:
boot_gdt_ptr:
        .word boot_gdt_end - boot_gdt - 1
        .long boot_gdt

        .text
long_mode_high:
        mov $0x10, %ax
        mov %ax, %ds
        mov %ax, %es
        mov %ax, %ss
        xor %ax, %ax
        mov %ax, %fs
        mov %ax, %gs
        movabs $boot_stack_top, %rsp
        xor %ebp, %ebp
        /* zero .bss; the stack itself is in .bss, so keep the Multiboot magic and info
         * address in r12/r13, which the C++ code called below must preserve (System V ABI) */
        mov %edi, %r12d
        mov %esi, %r13d
        movabs $__bss_start, %rdi
        movabs $__bss_end, %rcx
        sub %rdi, %rcx
        xor %eax, %eax
        rep stosb
        call run_global_constructors
        mov %r12d, %edi
        mov %r13d, %esi
        call kmain                          /* kmain(uint32_t magic, uint32_t info_phys) */
3:      cli
        hlt
        jmp 3b

        .section .bss
        .balign 16
boot_stack:
        .skip 16384
        .globl boot_stack_top
boot_stack_top:

        .section .note.GNU-stack, "", @progbits
