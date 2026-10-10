// elf_check.h - validate a 64-bit x86-64 ELF executable before loading it (B13).
// Every number below is a field offset or constant of the ELF-64 file format as this
// loader reads it (ELF specification / System V gABI and the AMD64 psABI; titles only:
// see the chapter's unverified box). Pure C++: the kernel and the host fuzzer
// (elf_fuzz.cpp) compile this same file.
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace k {

struct ElfSegment {
    uint64_t vaddr, memsz, offset, filesz;
    uint32_t flags;   // bit 0 execute, bit 1 write, bit 2 read
};

struct ElfImage {
    uint64_t entry;
    uint64_t phdr_vaddr;   // where the program headers appear in memory (0 if nowhere)
    uint16_t phnum;
    int nsegs;
    ElfSegment segs[8];
};

enum class ElfError {
    None, TooSmall, BadMagic, NotElf64, NotLittleEndian, BadVersion, NotExecutable,
    WrongMachine, BadHeaderSize, BadPhnum, PhdrsOutsideFile, NeedsInterpreter,
    TooManySegments, FileSizeAboveMemSize, SegmentOutsideFile, SegmentOutsideUser,
    SegmentMisaligned, SegmentsOverlap, NoLoadSegment, EntryNotExecutable,
};

inline const char* elf_error_name(ElfError e)
{
    static const char* const names[] = {
        "ok", "file too small", "bad magic", "not ELF64", "not little-endian",
        "bad version", "not an executable (ET_EXEC)", "not x86-64", "bad header sizes",
        "bad number of program headers", "program headers outside the file",
        "needs an interpreter (dynamic)", "too many segments", "filesz > memsz",
        "segment outside the file", "segment outside user space", "segment misaligned",
        "segments overlap", "no loadable segment", "entry point not in executable segment"};
    return names[static_cast<int>(e)];
}

namespace elf_detail {
// Little-endian reads, byte by byte: no alignment assumptions, no aliasing tricks.
inline uint64_t rd(const uint8_t* p, int n)
{
    uint64_t v = 0;
    for (int i = n - 1; i >= 0; --i) {
        v = (v << 8) | p[i];
    }
    return v;
}
}  // namespace elf_detail

// Checks the file f[0..size) and fills 'out'. Segments must lie inside [lo, hi).
// Every "a + b <= c" test is written as "b <= c - a" after checking a <= c, so that
// no sum can wrap around: a malicious file controls every one of these numbers.
inline ElfError elf_check(const uint8_t* f, uint64_t size, uint64_t lo, uint64_t hi, ElfImage& out)
{
    using elf_detail::rd;
    out = ElfImage{};
    if (size < 64) return ElfError::TooSmall;
    if (f[0] != 0x7F || f[1] != 'E' || f[2] != 'L' || f[3] != 'F') return ElfError::BadMagic;
    if (f[4] != 2) return ElfError::NotElf64;          // EI_CLASS = ELFCLASS64
    if (f[5] != 1) return ElfError::NotLittleEndian;   // EI_DATA = ELFDATA2LSB
    if (f[6] != 1 || rd(f + 20, 4) != 1) return ElfError::BadVersion;
    if (rd(f + 16, 2) != 2) return ElfError::NotExecutable;   // e_type = ET_EXEC
    if (rd(f + 18, 2) != 62) return ElfError::WrongMachine;   // e_machine = EM_X86_64
    uint64_t entry = rd(f + 24, 8), phoff = rd(f + 32, 8);
    uint64_t ehsize = rd(f + 52, 2), phentsize = rd(f + 54, 2), phnum = rd(f + 56, 2);
    if (ehsize != 64 || phentsize != 56) return ElfError::BadHeaderSize;
    if (phnum == 0 || phnum > 64) return ElfError::BadPhnum;
    if (phoff > size || phnum * 56 > size - phoff) return ElfError::PhdrsOutsideFile;
    out.entry = entry;
    out.phnum = uint16_t(phnum);
    for (uint64_t i = 0; i < phnum; ++i) {
        const uint8_t* ph = f + phoff + i * 56;
        uint32_t type = uint32_t(rd(ph, 4));
        if (type == 2 || type == 3) return ElfError::NeedsInterpreter;   // PT_DYNAMIC, PT_INTERP
        if (type != 1) continue;                                          // only PT_LOAD is loaded
        ElfSegment s{rd(ph + 16, 8), rd(ph + 40, 8), rd(ph + 8, 8), rd(ph + 32, 8),
                     uint32_t(rd(ph + 4, 4))};
        if (out.nsegs == 8) return ElfError::TooManySegments;
        if (s.filesz > s.memsz) return ElfError::FileSizeAboveMemSize;
        if (s.offset > size || s.filesz > size - s.offset) return ElfError::SegmentOutsideFile;
        if (s.vaddr < lo || s.vaddr > hi || s.memsz > hi - s.vaddr) return ElfError::SegmentOutsideUser;
        if (s.vaddr % 4096 != s.offset % 4096) return ElfError::SegmentMisaligned;
        for (int j = 0; j < out.nsegs; ++j) {
            const ElfSegment& o = out.segs[j];
            if (s.vaddr < o.vaddr + o.memsz && o.vaddr < s.vaddr + s.memsz) {
                return ElfError::SegmentsOverlap;   // sums are safe now: both are inside [lo, hi)
            }
        }
        if (phoff >= s.offset && phoff - s.offset < s.filesz) {
            out.phdr_vaddr = s.vaddr + (phoff - s.offset);
        }
        out.segs[out.nsegs++] = s;
    }
    if (out.nsegs == 0) return ElfError::NoLoadSegment;
    for (int j = 0; j < out.nsegs; ++j) {
        const ElfSegment& s = out.segs[j];
        if ((s.flags & 1) && entry >= s.vaddr && entry - s.vaddr < s.memsz) {
            return ElfError::None;
        }
    }
    return ElfError::EntryNotExecutable;
}

}  // namespace k
