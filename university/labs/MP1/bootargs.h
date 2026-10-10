// bootargs.h - MP1 starter: read "key=value" words from a kernel command line.
// Pure logic with no library underneath, so the same file is compiled into both kernels
// (x86-64: Multiboot cmdline; AArch64: /chosen/bootargs) and into the host test
// bootargs_test.cpp, which runs under AddressSanitizer and UBSan before any kernel boots.
#pragma once
#include <cstdint>

namespace bootargs {

// Finds the word "key=..." (words are separated by spaces) and parses a decimal value.
// Returns false if the key is missing, has no digits, or the value does not fit in 64 bits.
// "key" must match a whole word start: "xcpus=4" does not match the key "cpus".
inline bool get_u64(const char* line, const char* key, uint64_t* out)
{
    if (line == nullptr || key == nullptr) {
        return false;
    }
    for (const char* w = line; *w != '\0';) {
        while (*w == ' ') {
            ++w;
        }
        const char* k = key;
        const char* p = w;
        while (*k != '\0' && *p == *k) {
            ++p;
            ++k;
        }
        if (*k == '\0' && *p == '=') {
            ++p;
            if (*p < '0' || *p > '9') {
                return false;
            }
            uint64_t v = 0;
            for (; *p >= '0' && *p <= '9'; ++p) {
                uint64_t digit = static_cast<uint64_t>(*p - '0');
                if (v > (UINT64_MAX - digit) / 10) {
                    return false;                  // would overflow: refuse, never wrap
                }
                v = v * 10 + digit;
            }
            if (*p != ' ' && *p != '\0') {
                return false;                      // "cpus=4x" is not a number
            }
            *out = v;
            return true;
        }
        while (*w != ' ' && *w != '\0') {          // skip the rest of this word
            ++w;
        }
    }
    return false;
}

} // namespace bootargs
