// nx.cc - F11-10: NX / W^X. Data pages are mapped writable-but-not-executable.
// This program writes a tiny machine instruction (0xC3 = x86-64 RET) into a data
// array and then jumps to it. Because the array is in a non-executable page, the
// processor refuses to fetch an instruction from it and raises a fault; the OS
// turns that into SIGSEGV. This is why an attacker cannot simply drop code into a
// buffer and run it (and why real exploits turn to return-oriented programming).
#include <cstdio>

int main()
{
    // 0xC3 is "ret" on x86-64: a one-byte function that returns immediately. If
    // data were executable, calling this would do nothing and return.
    unsigned char code[16] = {0xC3};
    std::printf("about to call code in a data array...\n");
    std::fflush(stdout);

    using Fn = void (*)();
    Fn f = reinterpret_cast<Fn>(reinterpret_cast<void*>(code));
    f();                                   // fetch from a non-executable page -> fault

    std::printf("returned from data (NX is NOT enforced here)\n");
    return 0;
}
