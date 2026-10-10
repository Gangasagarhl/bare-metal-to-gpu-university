// u_priv.cc - try a privileged instruction in ring 3 (B12 test).
#include "ulib.h"

int main()
{
    print("priv: executing HLT in ring 3\n");
    asm volatile("hlt");   // privileged: the CPU raises a general-protection fault
    print("priv: still alive (this must never print)\n");
    return 0;
}
