// MP4 Listing 4: the kernel instances and the per-device tuning table (host code only).
// Rule 1: every device has an explicit row; an unknown device is an error, never a silent default.
// Rule 2: the row is chosen at run time from the device's own properties (F7-20), not from macros.
// Rule 3: every row names the evidence for its choice; "placeholder" rows must be replaced in M3.
#pragma once
#include <string>
#include "gemm_tile.hpp"

using Inst0 = TileCfg<64, 64, 16, 4, 4, 4>;    // the MP3 / F7-16 baseline: 256 work-items
using Inst1 = TileCfg<128, 64, 8, 8, 4, 4>;    // taller tile, more registers: 256 work-items
using Inst2 = TileCfg<32, 64, 16, 4, 4, 4>;    // smaller tile: 128 work-items

struct InstanceInfo
{
    const char* name;
    int threads;
    int bm, bn, bk, tm, tn, pad;
    int ldsBytes;
};

template <class Cfg>
constexpr InstanceInfo describe(const char* name)
{
    return {name, Cfg::THREADS, Cfg::BM, Cfg::BN, Cfg::BK, Cfg::TM, Cfg::TN, Cfg::PAD, Cfg::ldsBytes()};
}

inline constexpr InstanceInfo kInstances[] = {
    describe<Inst0>("i0_64x64x16_t4x4"),
    describe<Inst1>("i1_128x64x8_t8x4"),
    describe<Inst2>("i2_32x64x16_t4x4"),
};
inline constexpr int kInstanceCount = static_cast<int>(sizeof(kInstances) / sizeof(kInstances[0]));

struct TuningRow
{
    const char* arch;      // device key: AMD target name, or sm_XY on NVIDIA
    int waveWidth;         // expected warpSize; checked against the device at run time
    int instance;          // index into kInstances
    const char* evidence;  // measurement id that justifies the choice
};

inline constexpr TuningRow kTuning[] = {
    {"gfx90a", 64, 0, "placeholder: replace with the M3 measurement id"},
    {"gfx942", 64, 0, "placeholder: replace with the M3 measurement id"},
    {"gfx1100", 32, 2, "placeholder: replace with the M3 measurement id"},
    {"sm_80", 32, 0, "placeholder: MP3 measurement id of the NVIDIA GPU"},
};

// Device key from hipDeviceProp_t fields. AMD: gcnArchName up to the first ':' (feature
// suffixes are dropped). NVIDIA through HIP: gcnArchName is not filled, so use major/minor.
inline std::string archKey(const char* gcnArchName, int major, int minor, bool nvidia)
{
    if (nvidia) {
        return "sm_" + std::to_string(major) + std::to_string(minor);
    }
    std::string s(gcnArchName);
    const std::size_t colon = s.find(':');
    return colon == std::string::npos ? s : s.substr(0, colon);
}

inline const TuningRow* findTuning(const std::string& key)
{
    for (const TuningRow& r : kTuning) {
        if (key == r.arch) {
            return &r;
        }
    }
    return nullptr;   // the caller must stop with an error that names the key
}

// Lanes doing work / lanes allocated, for one workgroup (F7-20 worked example).
inline double laneUse(int threads, int wave)
{
    const int waves = (threads + wave - 1) / wave;
    return static_cast<double>(threads) / (waves * wave);
}
