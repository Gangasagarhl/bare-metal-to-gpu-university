// MP4 Listing 7: the shape matrix shared by the CPU suite and the GPU program.
// Rows 1-8 are MP3's matrix (kShapes in mp3_suite.cpp, MP3 Listing 2), copied; the run.sh step
// mp3_sync fails if the two ever differ. Rows 9-12 are MP4's: they cross the edges of the
// new, larger instances (BM up to 128) in every direction.
#pragma once
#include <cstdint>
#include <vector>

struct Shape
{
    int M, N, K;
};

inline const std::vector<Shape>& testShapes()
{
    static const std::vector<Shape> s = {
        {1, 1, 1},   {32, 32, 32}, {48, 48, 1},  {17, 13, 9},     // MP3
        {33, 31, 65}, {70, 5, 33},  {5, 70, 40},  {24, 36, 36},   // MP3
        {130, 130, 67}, {300, 40, 50}, {40, 300, 50}, {16, 16, 1000},   // MP4
    };
    return s;
}

// Deterministic inputs in [-1, 1) with full 24-bit significands, so products round.
inline std::vector<float> testMatrix(int rows, int cols, std::uint32_t seed)
{
    std::vector<float> m(static_cast<std::size_t>(rows) * cols);
    std::uint32_t x = seed;
    for (float& v : m) {
        x = x * 1664525u + 1013904223u;                       // linear congruential step
        v = static_cast<float>(x >> 8) / 8388608.0f - 1.0f;   // 24-bit value / 2^23 - 1
    }
    return m;
}
