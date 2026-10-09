// iotest.h - DR301 F4-05..F4-07: the random-I/O acceptance test, shared by the three
// storage drivers. The disk is seen as 4 KiB blocks. A xorshift32 generator chooses each
// operation; every write stores a pattern computed from (block, generation), so any read
// can be checked, and the host can check the final disk image with the same generator.
#pragma once
#include <stdint.h>

namespace iotest {
constexpr uint32_t kBlocks = 262144;               // 1 GiB / 4 KiB

inline uint32_t xorshift32(uint32_t& x)
{
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    return x;
}

inline void fill(uint8_t* buf, uint32_t block, uint32_t gen)
{
    auto* w = reinterpret_cast<uint32_t*>(buf);
    for (uint32_t i = 0; i < 1024; ++i) w[i] = block * 0x9E3779B1u + gen * 0x85EBCA77u + i * 0xC2B2AE3Du;
}

// The driver under test supplies these three operations (slot = request buffer index).
struct Disk {
    uint8_t* (*buffer)(int slot);
    void (*start)(int slot, bool write, uint32_t block);
    int (*wait_any)(bool& ok);                       // returns the finished slot
    int slots;
};

struct Result { uint32_t reads, writes, bad_reads, errors; uint64_t ms; };

// 'ops' operations at queue depth 'depth', generator seeded with 'seed'. gen[] (one byte
// per block, kBlocks long) remembers the last generation written; 0 = never written.
Result run(const Disk& d, uint32_t ops, int depth, uint32_t seed, uint8_t* gen);
}
