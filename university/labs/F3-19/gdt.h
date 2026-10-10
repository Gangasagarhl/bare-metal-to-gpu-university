// gdt.h - F3-19: the kernel's own Global Descriptor Table and 64-bit Task State Segment.
#pragma once
#include <cstdint>

namespace gdt {

// Selectors (index * 8, plus the requested privilege level for user selectors).
inline constexpr uint16_t kKernelCode = 0x08;
inline constexpr uint16_t kKernelData = 0x10;
inline constexpr uint16_t kUserData = 0x18 | 3;
inline constexpr uint16_t kUserCode = 0x20 | 3;
inline constexpr uint16_t kTss = 0x28;

// Interrupt Stack Table slots used by the IDT (slot numbers 1..7; 0 means "no IST").
inline constexpr uint8_t kIstDoubleFault = 1;
inline constexpr uint8_t kIstNmi = 2;
inline constexpr uint8_t kIstMachineCheck = 3;

struct Tss {
    uint32_t reserved0;
    uint64_t rsp[3];        // stacks for privilege levels 0..2 (used from F3-29 on)
    uint64_t reserved1;
    uint64_t ist[7];        // IST1..IST7
    uint64_t reserved2;
    uint16_t reserved3;
    uint16_t iomap_base;
} __attribute__((packed));
static_assert(sizeof(Tss) == 104, "the 64-bit TSS is 104 bytes");

// Encodes a code or data segment descriptor (the 8-byte format).
constexpr uint64_t segment(uint8_t access, uint8_t flags)
{
    // base 0 and limit 0xFFFFF: ignored for code/data in 64-bit mode, kept "flat" anyway
    return 0xFFFFull | (uint64_t(access) << 40) | (uint64_t(flags & 0xF) << 52) | (0xFull << 48);
}

// Access bytes: present | DPL | descriptor type 1 (code/data) | type bits.
inline constexpr uint8_t kAccKernelCode = 0x9A;   // present, DPL 0, code, execute/read
inline constexpr uint8_t kAccKernelData = 0x92;   // present, DPL 0, data, read/write
inline constexpr uint8_t kAccUserCode = 0xFA;     // present, DPL 3, code, execute/read
inline constexpr uint8_t kAccUserData = 0xF2;     // present, DPL 3, data, read/write
inline constexpr uint8_t kFlagsCode64 = 0xA;      // granularity 4 KiB, L = 1 (64-bit code)
inline constexpr uint8_t kFlagsData = 0xC;        // granularity 4 KiB, D/B = 1

void init(bool use_ist);    // builds the GDT and TSS, loads them (lgdt, ltr)
Tss& tss();

} // namespace gdt
