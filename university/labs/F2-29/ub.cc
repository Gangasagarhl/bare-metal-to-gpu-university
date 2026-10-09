// ub.cc: two kinds of undefined behaviour that UndefinedBehaviorSanitizer reports.
#include <cstdio>

int factorial(int n)
{
    int result = 1;
    for (int k = 2; k <= n; ++k) {
        result *= k;                          // overflows int for n = 13
    }
    return result;
}

unsigned mask(int bits)
{
    return (1u << bits) - 1u;                 // shifting a 32-bit value by 32 is undefined
}

int main()
{
    std::printf("13! = %d\n", factorial(13));
    std::printf("mask(32) = %u\n", mask(32));
    return 0;
}
