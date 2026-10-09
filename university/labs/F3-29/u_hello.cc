// u_hello.cc - the first user program: one system call to print, one to exit.
#include "ulib.h"

int main()
{
    print("hello from ring 3: pid ");
    print_num(sys_getpid());
    uint16_t cs;
    asm volatile("mov %%cs, %0" : "=r"(cs));
    print(", CS = ");
    print_hex(cs);
    print(" (low two bits = privilege level ");
    print_num(cs & 3);
    print(")\n");
    return 42;
}
