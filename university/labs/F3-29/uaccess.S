/* uaccess.S - memory accesses that are allowed to fault.
 * Each instruction that may touch a bad address is listed, with the address to resume
 * at, in the section .uaccess_fixups (an "exception table"). The page-fault handler
 * looks the faulting RIP up there (uaccess_fixup in syscall.cc); if found, it moves RIP
 * to the fixup code instead of crashing the kernel. */
    .text
/* uint64_t uaccess_copy(void* dst, const void* src, uint64_t n)
 * Returns the number of bytes NOT copied: 0 means success. */
    .global uaccess_copy
uaccess_copy:
    movq %rdx, %rcx
1:  rep movsb                  /* may fault; RCX then holds the bytes left */
    xorl %eax, %eax
    ret
2:  movq %rcx, %rax
    ret
    .section .uaccess_fixups, "a"
    .quad 1b, 2b
    .text

/* bool probe_read(const uint64_t* addr, uint64_t* out): one load that may fault
 * (used by the TLB shootdown test in F3-28). */
    .global probe_read
probe_read:
3:  movq (%rdi), %rax
    movq %rax, (%rsi)
    movl $1, %eax
    ret
4:  xorl %eax, %eax
    ret
    .section .uaccess_fixups, "a"
    .quad 3b, 4b
