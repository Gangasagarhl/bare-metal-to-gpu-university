// no_volatile.cpp: an rdtsc wrapper WITHOUT "volatile". Compare -O0 and -O2.
#include <cstdint>
#include <cstdio>

inline std::uint64_t rdtscNoVolatile()
{
    std::uint64_t value = 0;
    asm("rdtsc\n\t"                 // BUG: no volatile
        "shl $32, %%rdx\n\t"
        "or %%rdx, %%rax"
        : "=a"(value)
        :
        : "rdx");
    return value;
}

int main()
{
    const std::uint64_t t1 = rdtscNoVolatile();
    const std::uint64_t t2 = rdtscNoVolatile();
    std::printf("second reading minus first: %s\n",
                t2 == t1 ? "0 (the two readings are identical)" : "more than 0");
    return 0;
}
