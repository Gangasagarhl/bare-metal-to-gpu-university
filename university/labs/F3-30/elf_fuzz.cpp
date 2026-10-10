// elf_fuzz.cpp - fuzzing the kernel's ELF validator (elf_check.h) on the host under
// AddressSanitizer and UBSan. A minimal valid executable is built in memory, then
// mutated 200,000 times; every image the validator accepts is re-checked against the
// promises the loader relies on.
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "elf_check.h"

namespace {

constexpr uint64_t kLo = 0x0000008000000000ull, kHi = 0x0000010000000000ull;

void put(std::vector<uint8_t>& f, std::size_t at, uint64_t v, int n)
{
    for (int i = 0; i < n; ++i) f[at + static_cast<std::size_t>(i)] = uint8_t(v >> (8 * i));
}

// 64-byte ELF header + one 56-byte PT_LOAD program header + 16 bytes of "code".
std::vector<uint8_t> minimal_elf()
{
    std::vector<uint8_t> f(64 + 56 + 16, 0);
    const uint8_t ident[] = {0x7F, 'E', 'L', 'F', 2, 1, 1};
    std::memcpy(f.data(), ident, sizeof(ident));
    put(f, 16, 2, 2);                     // e_type = ET_EXEC
    put(f, 18, 62, 2);                    // e_machine = x86-64
    put(f, 20, 1, 4);                     // e_version
    put(f, 24, 0x8000400000 + 120, 8);    // e_entry: the code after the headers
    put(f, 32, 64, 8);                    // e_phoff
    put(f, 52, 64, 2);                    // e_ehsize
    put(f, 54, 56, 2);                    // e_phentsize
    put(f, 56, 1, 2);                     // e_phnum
    put(f, 64 + 0, 1, 4);                 // p_type = PT_LOAD
    put(f, 64 + 4, 5, 4);                 // p_flags = read + execute
    put(f, 64 + 8, 0, 8);                 // p_offset
    put(f, 64 + 16, 0x8000400000, 8);     // p_vaddr
    put(f, 64 + 32, f.size(), 8);         // p_filesz
    put(f, 64 + 40, f.size(), 8);         // p_memsz
    return f;
}

uint64_t rng = 0x2545F4914F6CDD1Dull;
uint64_t next()   // xorshift64: same sequence on every run
{
    rng ^= rng << 13;
    rng ^= rng >> 7;
    rng ^= rng << 17;
    return rng;
}

void mutate(std::vector<uint8_t>& f)
{
    int edits = 1 + int(next() % 4);
    for (int e = 0; e < edits; ++e) {
        switch (next() % 4) {
        case 0: f[next() % f.size()] = uint8_t(next()); break;                    // one random byte
        case 1: f[next() % f.size()] ^= uint8_t(1u << (next() % 8)); break;        // one bit
        case 2: {                                                                 // an interesting 64-bit value
            const uint64_t vals[] = {0, 1, 0xFFFFFFFFFFFFFFFFull, 0x8000000000000000ull, kHi, kHi - 1,
                                     kLo, 0x7FFFFFFF, 0x100000};
            std::size_t at = next() % (f.size() - 7);
            put(f, at, vals[next() % (sizeof(vals) / sizeof(vals[0]))], 8);
            break;
        }
        default: f.resize(next() % (f.size() + 1)); if (f.size() < 8) f.resize(8); break;   // truncate
        }
    }
}

}  // namespace

int main()
{
    std::vector<uint8_t> base = minimal_elf();
    k::ElfImage img{};
    k::ElfError e = k::elf_check(base.data(), base.size(), kLo, kHi, img);
    std::printf("minimal ELF (%zu bytes): %s, entry 0x%llx, %d segment(s)\n", base.size(),
                k::elf_error_name(e), static_cast<unsigned long long>(img.entry), img.nsegs);
    if (e != k::ElfError::None) return 1;

    constexpr int kRuns = 200000;
    long hist[32] = {};
    long broken = 0;
    for (int r = 0; r < kRuns; ++r) {
        std::vector<uint8_t> f = base;
        mutate(f);
        // Copy into an exactly-sized heap block so ASan sees any read past the end.
        std::vector<uint8_t> exact(f.begin(), f.end());
        e = k::elf_check(exact.data(), exact.size(), kLo, kHi, img);
        ++hist[static_cast<int>(e)];
        if (e != k::ElfError::None) continue;
        bool entry_ok = false;
        for (int j = 0; j < img.nsegs; ++j) {   // the loader's promises
            const k::ElfSegment& s = img.segs[j];
            if (s.offset > exact.size() || s.filesz > exact.size() - s.offset) ++broken;
            if (s.vaddr < kLo || s.vaddr > kHi || s.memsz > kHi - s.vaddr) ++broken;
            if ((s.flags & 1) && img.entry >= s.vaddr && img.entry - s.vaddr < s.memsz) entry_ok = true;
        }
        if (!entry_ok) ++broken;
    }
    std::printf("%d mutated files:\n", kRuns);
    for (int i = 0; i < 20; ++i) {
        if (hist[i] != 0) std::printf("  %7ld  %s\n", hist[i], k::elf_error_name(static_cast<k::ElfError>(i)));
    }
    std::printf("accepted files that break a loader promise: %ld -> %s\n", broken, broken == 0 ? "PASS" : "FAIL");
    return broken == 0 ? 0 : 1;
}
