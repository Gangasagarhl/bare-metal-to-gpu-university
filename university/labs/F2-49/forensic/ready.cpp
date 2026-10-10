// ready.cpp: a driver helper marks a buffer "ready" with a single store instruction written
// in inline assembly (the team wanted "exactly one 32-bit store"), then checks the flag.
#include <cstdio>

struct Buffer {
    int ready;
    int length;
};

__attribute__((noinline)) void markReady(Buffer* b)
{
    asm volatile("movl $1, (%0)" : : "r"(&b->ready));
}

__attribute__((noinline)) int publish(Buffer* b)
{
    b->ready = 0;
    b->length = 64;
    asm volatile("movl $1, (%0)" : : "r"(&b->ready));   // inlined copy of markReady
    return b->ready;                                      // 1 expected
}

int main()
{
    Buffer b{};
    const int seen = publish(&b);
    markReady(&b);
    std::printf("publish() saw ready = %d; memory now holds ready = %d\n", seen, b.ready);
    return seen == 1 ? 0 : 1;
}
