// hello_libc.cc - F3-50: the same greeting through the C library, for comparison with
// nolibc_hello.cc. Built twice by run.sh: statically and dynamically linked.
#include <cstdio>

int main()
{
    std::printf("hello from x86-64 through the C library\n");
    return 0;
}
