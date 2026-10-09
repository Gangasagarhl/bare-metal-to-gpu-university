// ulib.cc - the two functions the compiler may call on its own in user programs.
#include <stddef.h>

extern "C" void* memset(void* d, int c, size_t n)
{
    void* p = d;
    asm volatile("rep stosb" : "+D"(p), "+c"(n) : "a"(c) : "memory");
    return d;
}

extern "C" void* memcpy(void* d, const void* s, size_t n)
{
    void* p = d;
    asm volatile("rep movsb" : "+D"(p), "+S"(s), "+c"(n) : : "memory");
    return d;
}
