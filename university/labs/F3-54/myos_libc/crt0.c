/* crt0.c - F3-54 myos sysroot: the start file. The kernel enters at _start with argc at the
   stack pointer, argv after it (the same initial stack as Linux on x86-64, because this
   teaching port chose the Linux-compatible system-call ABI of F3-50, path 2). */
int main(int argc, char** argv);
_Noreturn void exit(int status);

__asm__(".globl _start\n_start:\n"
        "  xor %rbp, %rbp\n"
        "  mov %rsp, %rdi\n"
        "  and $-16, %rsp\n"
        "  call __myos_start\n"
        "  hlt\n");

_Noreturn void __myos_start(long* sp)
{
    int argc = (int)sp[0];
    char** argv = (char**)(sp + 1);
    exit(main(argc, argv));
}
