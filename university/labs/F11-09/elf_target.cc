// elf_target.cc - a cut-down ELF64 section reader, the fuzzing target of the
// F11-09 lab (curriculum P2: "a fuzzing run of at least an hour finds nothing").
// It is the same kind of code as the P2 reader of SP301 (F2-44), reduced to the
// one path the fuzzer attacks: reading each section's NAME from the section-name
// string table (.shstrtab). The header reads are bounds-checked; the NAME read
// is NOT, unless this file is compiled with -DFIXED. Compiled with
// -fsanitize-coverage=trace-pc so the fuzzer sees which branches it reaches.
#include "fuzz.h"
#include <cstdint>
#include <cstring>
#include <vector>

namespace {

// Safe little-endian reads: refuse anything that would fall outside [0, size).
bool rd_u16(const unsigned char* p, unsigned long size, unsigned long off, uint16_t& v)
{
    if (off > size || size - off < 2) return false;
    v = static_cast<uint16_t>(p[off] | (p[off + 1] << 8));
    return true;
}
bool rd_u32(const unsigned char* p, unsigned long size, unsigned long off, uint32_t& v)
{
    if (off > size || size - off < 4) return false;
    v = static_cast<uint32_t>(p[off]) | (static_cast<uint32_t>(p[off + 1]) << 8)
      | (static_cast<uint32_t>(p[off + 2]) << 16) | (static_cast<uint32_t>(p[off + 3]) << 24);
    return true;
}
bool rd_u64(const unsigned char* p, unsigned long size, unsigned long off, uint64_t& v)
{
    uint32_t lo, hi;
    if (!rd_u32(p, size, off, lo) || !rd_u32(p, size, off + 4, hi)) return false;
    v = lo | (static_cast<uint64_t>(hi) << 32);
    return true;
}

}  // namespace

int fuzz_one(const unsigned char* data, unsigned long size)
{
    // Parse a private, exactly sized heap copy so that any over-read is a clean
    // heap-buffer-overflow that AddressSanitizer can pinpoint.
    std::vector<unsigned char> owned(data, data + size);
    const unsigned char* p = owned.data();

    // ELF64 identification: magic 0x7f 'E' 'L' 'F', class 2, little-endian.
    if (size < 64) return 0;
    if (!(p[0] == 0x7f && p[1] == 'E' && p[2] == 'L' && p[3] == 'F')) return 0;
    if (p[4] != 2 || p[5] != 1) return 0;                       // ELFCLASS64, little-endian only

    uint64_t e_shoff = 0;
    uint16_t e_shentsize = 0, e_shnum = 0, e_shstrndx = 0;
    if (!rd_u64(p, size, 40, e_shoff)) return 0;
    if (!rd_u16(p, size, 58, e_shentsize)) return 0;
    if (!rd_u16(p, size, 60, e_shnum)) return 0;
    if (!rd_u16(p, size, 62, e_shstrndx)) return 0;
    if (e_shnum == 0 || e_shentsize < 64) return 0;             // nothing to walk

    // Locate the section-name string table header (one section header entry).
    uint64_t str_hdr = e_shoff + static_cast<uint64_t>(e_shstrndx) * e_shentsize;
    uint64_t strtab_off = 0, strtab_size = 0;
    if (!rd_u64(p, size, str_hdr + 24, strtab_off)) return 0;   // sh_offset
    if (!rd_u64(p, size, str_hdr + 32, strtab_size)) return 0;  // sh_size
    (void)strtab_size;

    // Walk every section header and read its name from the string table.
    for (uint64_t i = 0; i < e_shnum; ++i) {
        uint64_t hdr = e_shoff + i * e_shentsize;
        uint32_t sh_name = 0;
        if (!rd_u32(p, size, hdr, sh_name)) break;              // header itself is bounds-checked

        uint64_t name_at = strtab_off + sh_name;                // both fields come from the file
#ifdef FIXED
        // The fix: the name must lie inside the string table, which must lie
        // inside the file. Without this, a crafted sh_name or sh_offset walks
        // off the end of the buffer.
        if (strtab_off > size || name_at >= size) continue;
        uint64_t limit = size;
#else
        uint64_t limit = ~0ull;                                 // BUG: no bound on the read
#endif
        // Read the NUL-terminated name. In the buggy build `limit` is enormous,
        // so this loop can run past the end of `owned`.
        volatile unsigned char sink = 0;
        for (uint64_t j = name_at; j < limit && p[j] != 0; ++j) sink = static_cast<unsigned char>(sink ^ p[j]);
        (void)sink;
    }
    return 0;
}
