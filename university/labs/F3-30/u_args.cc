// u_args.cc - print what the kernel put on the initial stack (B13 worked example).
#include "ulib.h"

extern "C" char _start[];

int main(int argc, char** argv)
{
    print("args: argc = ");
    print_num(argc);
    print("\n");
    for (int i = 0; i < argc; ++i) {
        print("args: argv[");
        print_num(i);
        print("] = \"");
        print(argv[i]);
        print("\" at ");
        print_hex(reinterpret_cast<uint64_t>(argv[i]));
        print("\n");
    }
    char** envp = argv + argc + 1;   // after argv's NULL
    while (*envp != nullptr) {
        ++envp;
    }
    auto* aux = reinterpret_cast<uint64_t*>(envp + 1);
    for (; aux[0] != 0; aux += 2) {
        print("args: auxv type ");
        print_num(int64_t(aux[0]));
        print(" value ");
        print_hex(aux[1]);
        print("\n");
    }
    print("args: _start is at ");
    print_hex(reinterpret_cast<uint64_t>(_start));
    print(", argv itself at ");
    print_hex(reinterpret_cast<uint64_t>(argv));
    print("\n");
    return 0;
}
