// call_asm.cpp: C++ calls the hand-written assembly function sum6 through the ABI.
#include <cstdio>

extern "C" long sum6(long a, long b, long c, long d, long e, long f);

int main()
{
    const long r = sum6(1, 20, 300, 4000, 50000, 600000);
    std::printf("sum6(1, 20, 300, 4000, 50000, 600000) = %ld\n", r);
    return r == 654321 ? 0 : 1;
}
