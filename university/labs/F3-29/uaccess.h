// uaccess.h - where user memory lives, and the range check every system call applies
// to a pointer it receives from user mode. Pure functions: the host test
// (uaccess_test.cpp) runs the very same code under AddressSanitizer and UBSan.
#pragma once
#include <stdint.h>

namespace k {

// User half of every address space in this kernel: PML4 entry 1, 512 GiB to 1 TiB.
constexpr uint64_t kUserBase = 0x0000008000000000ull;
constexpr uint64_t kUserTop = 0x0000010000000000ull;   // first address above user space

// True if [addr, addr + len) lies entirely inside user space. Written so that
// addr + len is never computed: that sum can wrap around 2^64.
constexpr bool user_range_ok(uint64_t addr, uint64_t len)
{
    if (addr < kUserBase || addr > kUserTop) {
        return false;
    }
    return len <= kUserTop - addr;
}

// The tempting version, kept here only so the test can show why it is wrong.
constexpr bool user_range_naive(uint64_t addr, uint64_t len)
{
    return addr >= kUserBase && addr + len <= kUserTop;
}

}  // namespace k
