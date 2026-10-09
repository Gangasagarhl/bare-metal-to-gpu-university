// F1-44: a write-back data cache in front of memory, and a DMA engine that
// talks to memory directly (a NON-coherent system: the cache never sees DMA).
// 64 bytes of memory, 16-byte cache lines, so 4 lines; exercise sizes only.
#pragma once
#include <array>
#include <cstdint>
#include <cstdio>

constexpr std::size_t kMem = 64;
constexpr std::size_t kLine = 16;

struct System
{
    std::array<std::uint8_t, kMem> mem{};
    std::array<std::array<std::uint8_t, kLine>, kMem / kLine> line{};
    std::array<bool, kMem / kLine> valid{};
    std::array<bool, kMem / kLine> dirty{};

    std::uint8_t cpu_read(std::size_t a)
    {
        const std::size_t l = a / kLine;
        if (!valid[l]) {                       // miss: fill the whole line from memory
            for (std::size_t i = 0; i < kLine; ++i) line[l][i] = mem[l * kLine + i];
            valid[l] = true;
        }
        return line[l][a % kLine];
    }
    void cpu_write(std::size_t a, std::uint8_t v)
    {
        cpu_read(a);                           // write-allocate
        line[a / kLine][a % kLine] = v;
        dirty[a / kLine] = true;               // memory is NOT updated yet
    }
    void clean(std::size_t a, std::size_t n)   // write dirty lines back to memory
    {
        for (std::size_t l = a / kLine; l <= (a + n - 1) / kLine; ++l) {
            if (valid[l] && dirty[l]) {
                for (std::size_t i = 0; i < kLine; ++i) mem[l * kLine + i] = line[l][i];
                dirty[l] = false;
            }
        }
    }
    void invalidate(std::size_t a, std::size_t n)   // forget cached copies
    {
        for (std::size_t l = a / kLine; l <= (a + n - 1) / kLine; ++l) valid[l] = false;
    }
    void dma_write(std::size_t a, std::uint8_t v) { mem[a] = v; }   // device -> memory
    std::uint8_t dma_read(std::size_t a) const { return mem[a]; }   // memory -> device
};

inline void dump(const char* who, const std::uint8_t* p, std::size_t n)
{
    std::printf("%-26s", who);
    for (std::size_t i = 0; i < n; ++i) std::printf("%02X%s", p[i], (i % 16 == 15) ? " | " : " ");
    std::printf("\n");
}
