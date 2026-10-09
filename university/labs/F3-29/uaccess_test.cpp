// uaccess_test.cpp - the kernel's user-pointer range check (uaccess.h), compiled for the
// host and run under AddressSanitizer and UBSan, next to the tempting "naive" version.
#include <cstdint>
#include <cstdio>

#include "uaccess.h"

using k::kUserBase;
using k::kUserTop;

struct Case {
    const char* what;
    uint64_t addr, len;
    bool want;   // should the system call accept this buffer?
};

int main()
{
    const Case cases[] = {
        {"start of user space, 16 bytes", kUserBase, 16, true},
        {"last byte of user space", kUserTop - 1, 1, true},
        {"one byte past the end", kUserTop - 1, 2, false},
        {"kernel address (1 MiB)", 0x100000, 16, false},
        {"null pointer", 0, 8, false},
        {"zero length at the top", kUserTop, 0, true},
        {"user address, length wraps around 2^64", kUserBase, UINT64_MAX - kUserBase + 0x200000, false},
        {"huge length", kUserBase + 4096, UINT64_MAX, false},
        {"non-canonical address", 0x8000000000000000ull, 8, false},
    };
    int wrong_ok = 0, wrong_naive = 0;
    std::printf("%-40s %-6s %-6s %-6s\n", "buffer", "want", "ok()", "naive()");
    for (const Case& c : cases) {
        bool ok = k::user_range_ok(c.addr, c.len);
        bool naive = k::user_range_naive(c.addr, c.len);
        wrong_ok += ok != c.want;
        wrong_naive += naive != c.want;
        std::printf("%-40s %-6s %-6s %-6s%s\n", c.what, c.want ? "yes" : "no", ok ? "yes" : "no",
                    naive ? "yes" : "no", naive != c.want ? "   <- naive check is wrong" : "");
    }
    std::printf("user_range_ok wrong in %d cases, user_range_naive wrong in %d cases\n", wrong_ok, wrong_naive);
    return wrong_ok == 0 ? 0 : 1;
}
