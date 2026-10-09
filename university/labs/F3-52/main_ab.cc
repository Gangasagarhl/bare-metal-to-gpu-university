// main_ab.cc - F3-52: a program linked against libb.so, which itself depends on liba.so.
// U2 acceptance test, first part: the chain loads, and a symbol defined in the executable
// wins over the same symbol in a library (interposition, as the System V gABI describes).
#include <cstdio>

extern "C" int answer();
extern "C" const char* who_does_libb_see();

extern "C" const char* who()            // the executable's own definition
{
    return "the executable";
}

int main()
{
    std::printf("answer() from libb.so (uses liba.so) = %d\n", answer());
    std::printf("libb.so's call to who() reached: %s\n", who_does_libb_see());
    return 0;
}
