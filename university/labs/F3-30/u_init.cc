// u_init.cc - the first process: start ten children, wait for all of them (B13).
#include "ulib.h"

int main(int argc, char** argv)
{
    print("init: pid ");
    print_num(sys_getpid());
    print(", argv[0] = ");
    print(argc > 0 ? argv[0] : "?");
    print("\n");
    for (int i = 0; i < 10; ++i) {
        char num[2] = {char('0' + i), '\0'};
        const char* av[] = {"u_child.elf", num, nullptr};
        int64_t pid = sys_spawn("u_child.elf", av);
        if (pid < 0) {
            print("init: spawn failed: ");
            print_num(pid);
            print("\n");
            return 1;
        }
    }
    uint32_t seen = 0;
    int reaped = 0;
    for (;;) {
        int status = -1;
        int64_t pid = sys_wait(&status);
        if (pid < 0) {
            break;   // no children left
        }
        print("init: child pid ");
        print_num(pid);
        print(" exited with status ");
        print_num(status);
        print("\n");
        if (status >= 10 && status < 20) {
            seen |= 1u << (status - 10);
        }
        ++reaped;
    }
    print("init: reaped ");
    print_num(reaped);
    print(" children, ten distinct statuses: ");
    print(seen == 0x3FF ? "yes\n" : "NO\n");
    return seen == 0x3FF && reaped == 10 ? 0 : 1;
}
