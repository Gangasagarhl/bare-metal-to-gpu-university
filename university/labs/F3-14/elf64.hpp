// elf64.hpp - check an ELF64 kernel image and list what must be loaded (its PT_LOAD segments).
// Freestanding: no standard library, no exceptions, no allocation, so the same code runs in the
// host test tool (elfinfo.cc, F3-14) and in the UEFI loader (loader.cc, F3-15).
// Field offsets were written from memory of the ELF specification (Tool Interface Standard)
// and the System V AMD64 psABI; they are pending verification (see F3-14's unverified box).
#pragma once
#include <stddef.h>
#include <stdint.h>

namespace elf {

constexpr uint32_t kPtLoad = 1;
constexpr uint32_t kFlagX = 1, kFlagW = 2, kFlagR = 4;
constexpr uint64_t kHigherHalf = 0xffffffff80000000ull;  // our protocol: kernels live here
constexpr int kMaxSegments = 8;

struct Segment {
    uint64_t offset;  // where the bytes are in the file
    uint64_t vaddr;   // where the kernel expects them in virtual memory
    uint64_t filesz;  // bytes to copy from the file
    uint64_t memsz;   // bytes in memory (the rest, memsz - filesz, is zero-filled: .bss)
    uint32_t flags;   // kFlagR | kFlagW | kFlagX
};

struct Image {
    uint64_t entry = 0;
    int count = 0;
    Segment seg[kMaxSegments] = {};
    uint64_t lowest = 0, highest = 0;  // virtual span [lowest, highest) of all segments
};

inline uint64_t get(const uint8_t* p, int bytes)  // little-endian read
{
    uint64_t v = 0;
    for (int i = bytes - 1; i >= 0; --i) {
        v = v << 8 | p[i];
    }
    return v;
}

// Returns nullptr when the image is acceptable, otherwise a short reason. Never reads outside
// [data, data + size), whatever the file contains.
inline const char* check(const uint8_t* data, uint64_t size, Image* out)
{
    if (size < 64) {
        return "file shorter than an ELF64 header";
    }
    if (data[0] != 0x7f || data[1] != 'E' || data[2] != 'L' || data[3] != 'F') {
        return "bad ELF magic";
    }
    if (data[4] != 2 || data[5] != 1) {
        return "not a 64-bit little-endian ELF file";
    }
    if (get(data + 16, 2) != 2 || get(data + 18, 2) != 62) {
        return "not an x86-64 executable (e_type 2, e_machine 62 expected)";
    }
    const uint64_t phoff = get(data + 32, 8), phentsize = get(data + 54, 2), phnum = get(data + 56, 2);
    if (phentsize != 56 || phoff > size || phnum > (size - phoff) / 56) {
        return "program header table outside the file";
    }
    out->entry = get(data + 24, 8);
    out->count = 0;
    for (uint64_t i = 0; i < phnum; ++i) {
        const uint8_t* ph = data + phoff + i * 56;
        if (get(ph, 4) != kPtLoad) {
            continue;
        }
        if (out->count == kMaxSegments) {
            return "too many PT_LOAD segments";
        }
        Segment s{get(ph + 8, 8), get(ph + 16, 8), get(ph + 32, 8), get(ph + 40, 8),
                  static_cast<uint32_t>(get(ph + 4, 4))};
        if (s.offset > size || s.filesz > size - s.offset) {
            return "segment data outside the file";
        }
        if (s.filesz > s.memsz || s.vaddr < kHigherHalf || s.memsz > ~uint64_t{0} - s.vaddr) {
            return "segment size or address not acceptable (must be in the higher half)";
        }
        if ((s.vaddr & 0xfff) != 0) {
            return "segment not page-aligned";
        }
        for (int k = 0; k < out->count; ++k) {
            const Segment& o = out->seg[k];
            const uint64_t o_end = (o.vaddr + o.memsz + 0xfff) & ~uint64_t{0xfff};
            const uint64_t s_end = (s.vaddr + s.memsz + 0xfff) & ~uint64_t{0xfff};
            if (s.vaddr < o_end && o.vaddr < s_end) {
                return "segments overlap";
            }
        }
        out->seg[out->count++] = s;
    }
    if (out->count == 0) {
        return "no PT_LOAD segment";
    }
    out->lowest = ~uint64_t{0};
    out->highest = 0;
    bool entry_ok = false;
    for (int k = 0; k < out->count; ++k) {
        const Segment& s = out->seg[k];
        out->lowest = s.vaddr < out->lowest ? s.vaddr : out->lowest;
        const uint64_t end = (s.vaddr + s.memsz + 0xfff) & ~uint64_t{0xfff};
        out->highest = end > out->highest ? end : out->highest;
        entry_ok = entry_ok || ((s.flags & kFlagX) && out->entry >= s.vaddr && out->entry < s.vaddr + s.memsz);
    }
    if (!entry_ok) {
        return "entry point not inside an executable segment";
    }
    return nullptr;
}

}  // namespace elf
