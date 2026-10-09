// div64.cc - DR301: 64-bit division for the 32-bit lab kernel.
// On a 32-bit x86 target, g++ turns a 64-bit '/' or '%' into calls to helper functions
// that normally come from libgcc. The kernel links without libgcc, so it supplies them:
// plain shift-and-subtract long division, one quotient bit per step.
#include <stdint.h>

extern "C" uint64_t __udivmoddi4(uint64_t n, uint64_t d, uint64_t* rem)
{
    if (d == 0) {                         // the CPU would trap; the kernel just stops
        for (;;) asm volatile("cli; hlt");
    }
    uint64_t q = 0, r = 0;
    for (int i = 63; i >= 0; --i) {
        r = (r << 1) | ((n >> i) & 1);
        if (r >= d) { r -= d; q |= uint64_t{1} << i; }
    }
    if (rem) *rem = r;
    return q;
}

extern "C" uint64_t __udivdi3(uint64_t n, uint64_t d) { return __udivmoddi4(n, d, nullptr); }

extern "C" uint64_t __umoddi3(uint64_t n, uint64_t d)
{
    uint64_t r;
    __udivmoddi4(n, d, &r);
    return r;
}

// Signed versions: divide the magnitudes; C++ rounds the quotient toward zero and gives
// the remainder the sign of the dividend.
extern "C" int64_t __divdi3(int64_t n, int64_t d)
{
    const bool neg = (n < 0) != (d < 0);
    const uint64_t q = __udivmoddi4(n < 0 ? 0 - static_cast<uint64_t>(n) : static_cast<uint64_t>(n),
                                    d < 0 ? 0 - static_cast<uint64_t>(d) : static_cast<uint64_t>(d), nullptr);
    return neg ? -static_cast<int64_t>(q) : static_cast<int64_t>(q);
}

extern "C" int64_t __moddi3(int64_t n, int64_t d)
{
    uint64_t r;
    __udivmoddi4(n < 0 ? 0 - static_cast<uint64_t>(n) : static_cast<uint64_t>(n),
                 d < 0 ? 0 - static_cast<uint64_t>(d) : static_cast<uint64_t>(d), &r);
    return n < 0 ? -static_cast<int64_t>(r) : static_cast<int64_t>(r);
}
