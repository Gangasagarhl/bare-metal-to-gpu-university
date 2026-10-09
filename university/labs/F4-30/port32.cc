// port32.cc - F4-30: what a 32-bit port must add so that DR402's arch-neutral code links.
// The neutral code divides 64-bit numbers (kformat's %lu/%lx, the frame allocator) and uses
// std::atomic<uint64_t>. A 64-bit CPU does both in instructions. A 32-bit CPU's compiler
// calls helper functions instead, normally from libgcc / compiler-rt and libatomic, which a
// freestanding kernel does not link. The names and signatures below are the ones the
// compilers call (recalled from memory: chapter F4-30, unverified box); the run checks
// that the link succeeds and that the tests then compute the right values.
#include <cstdint>

namespace {

// Shift-and-subtract division: slow but short, and needs no division instruction at all.
uint64_t udivmod64(uint64_t n, uint64_t d, uint64_t* rem)
{
    if (d == 0) {
        *rem = n;
        return ~uint64_t{0};          // a kernel would panic here
    }
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) {
            r -= d;
            q |= uint64_t{1} << i;
        }
    }
    *rem = r;
    return q;
}

uint64_t magnitude(int64_t v)
{
    return v < 0 ? uint64_t{0} - static_cast<uint64_t>(v) : static_cast<uint64_t>(v);
}

} // namespace

extern "C" {

// libgcc names (i386, MIPS, PowerPC, RISC-V 32)
uint64_t __udivdi3(uint64_t n, uint64_t d)
{
    uint64_t r;
    return udivmod64(n, d, &r);
}
uint64_t __umoddi3(uint64_t n, uint64_t d)
{
    uint64_t r;
    udivmod64(n, d, &r);
    return r;
}
int64_t __divdi3(int64_t n, int64_t d)
{
    uint64_t r;
    uint64_t q = udivmod64(magnitude(n), magnitude(d), &r);
    return (n < 0) != (d < 0) ? -static_cast<int64_t>(q) : static_cast<int64_t>(q);
}
int64_t __moddi3(int64_t n, int64_t d)
{
    uint64_t r;
    udivmod64(magnitude(n), magnitude(d), &r);
    return n < 0 ? -static_cast<int64_t>(r) : static_cast<int64_t>(r);
}

uint64_t f430_udivmod64(uint64_t n, uint64_t d, uint64_t* rem)
{
    return udivmod64(n, d, rem);
}

// 64-bit atomics where the CPU has no 64-bit atomic instruction (MIPS32, PowerPC 32, RV32).
// These are NOT atomic: correct only on one CPU with interrupts masked, which is all this
// single-threaded test needs. A real port takes a lock here, or keeps 64-bit atomics out of
// core code altogether (the neutrality checklist, F4-30).
uint64_t __atomic_load_8(const volatile void* p, int)
{
    return *static_cast<const volatile uint64_t*>(p);
}
void __atomic_store_8(volatile void* p, uint64_t v, int)
{
    *static_cast<volatile uint64_t*>(p) = v;
}
uint64_t __atomic_fetch_add_8(volatile void* p, uint64_t v, int)
{
    auto* x = static_cast<volatile uint64_t*>(p);
    uint64_t old = *x;
    *x = old + v;
    return old;
}
bool __atomic_compare_exchange_8(volatile void* p, void* expected, uint64_t desired, int, int)
{
    auto* x = static_cast<volatile uint64_t*>(p);
    auto* e = static_cast<uint64_t*>(expected);
    if (*x == *e) {
        *x = desired;
        return true;
    }
    *e = *x;
    return false;
}

} // extern "C"

#if defined(__arm__)
// The Arm EABI division helper returns the quotient in r0:r1 AND the remainder in r2:r3,
// which C cannot express: a few instructions move the remainder into place.
asm(R"(
        .globl __aeabi_uldivmod
        .type __aeabi_uldivmod, %function
__aeabi_uldivmod:
        push    {r4, lr}
        sub     sp, sp, #16
        add     r4, sp, #8
        str     r4, [sp]            @ third argument (remainder pointer) goes on the stack
        bl      f430_udivmod64      @ r0:r1 = quotient
        ldrd    r2, r3, [sp, #8]    @ r2:r3 = remainder
        add     sp, sp, #16
        pop     {r4, pc}
)");
#endif
