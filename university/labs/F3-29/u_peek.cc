// u_peek.cc - read kernel memory directly from ring 3 (the page is supervisor-only).
#include "ulib.h"

int main()
{
    print("peek: reading kernel address 0x100000\n");
    volatile uint64_t* p = reinterpret_cast<volatile uint64_t*>(0x100000);
    print_hex(*p);
    print(" (this must never print)\n");
    return 0;
}
