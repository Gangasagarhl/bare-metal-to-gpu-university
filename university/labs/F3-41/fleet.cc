// fleet.cc - F3-41 forensic evidence generator: "The update that bricked half the fleet"
// (simulated). 1,000 devices receive firmware 2 over a slow link; each download-and-write
// takes 20 minutes, spread evenly over the image's flash operations. Devices start at random
// times between 01:40 and 02:20. The building's power is off from 02:00 to 02:10.
// "fleet single" replays the night with the fleet's real design (SingleSlot);
// "fleet ab" replays exactly the same night (same seed) with ABSlots.
#include "ota.h"

namespace {

const Pkey vendorKey = keyFromSeed("OS305 vendor signing key");
const Bytes v1 = makeImage(1, 6000, vendorKey);
const Bytes v2 = makeImage(2, 6200, vendorKey);

uint32_t rng = 20261009;              // fixed seed: the same night in every run and both designs
uint32_t nextRandom() { rng = rng * 1664525u + 1013904223u; return rng >> 8; }

std::string clock(double minutes)                         // minutes after midnight -> hh:mm:ss
{
    const int s = static_cast<int>(minutes * 60 + 0.5);
    char b[16];
    std::snprintf(b, sizeof b, "%02d:%02d:%02d", s / 3600, (s / 60) % 60, s % 60);
    return b;
}

template <typename Device>
Device makeDevice()
{
    if constexpr (std::is_same_v<Device, ABSlots>) { return Device(vendorKey); } else { return Device(); }
}

template <typename Device>
void night(const char* title, bool printSamples)
{
    constexpr double kCutStart = 120.0, kCutEnd = 130.0, kDuration = 20.0;
    int onV1 = 0, onV2 = 0, bricked = 0, interrupted = 0;
    int bucketTotal[8] = {}, bucketBricked[8] = {};       // by 5-minute start window
    std::printf("=== %s ===\n", title);
    for (int id = 0; id < 1000; ++id) {
        Device d = makeDevice<Device>();
        d.factory(v1); d.boot(); d.log.clear();
        double start = 100.0 + (nextRandom() % 2400) / 60.0;   // 01:40:00 .. 02:19:59
        if (start >= kCutStart && start < kCutEnd) { start = kCutEnd; }   // waits for power
        const long before = d.flash.ops;
        Device probe = makeDevice<Device>();               // count the update's operations
        probe.factory(v1); probe.boot();
        const long p0 = probe.flash.ops;
        probe.update(v2);
        const long n = probe.flash.ops - p0;
        bool cut = false;
        long cutOp = 0;
        if (start < kCutStart && start + kDuration > kCutStart) {
            cutOp = static_cast<long>((kCutStart - start) / kDuration * n);
            d.flash.cutAt = before + cutOp;
            cut = true;
            ++interrupted;
        }
        d.update(v2);
        d.flash.restorePower();
        int v = d.boot();
        if (v > 0) { d.confirm(); v = d.boot(); }
        (v == 1 ? onV1 : v == 2 ? onV2 : bricked) += 1;
        const int bucket = static_cast<int>((start - 100.0) / 5.0);
        bucketTotal[bucket] += 1;
        bucketBricked[bucket] += v < 0 ? 1 : 0;
        if (printSamples && (id < 6)) {
            char note[64] = "";
            if (cut) { std::snprintf(note, sizeof note, " [power lost at operation %ld of %ld]", cutOp + 1, n); }
            std::printf("dev %04d start %s%s: %s=> %s\n", id, clock(start).c_str(), note, d.log.c_str(),
                        v < 0 ? "BRICKED" : v == 2 ? "running v2" : "running v1, update to retry");
        }
    }
    std::printf("start window   devices  bricked\n");
    for (int b = 0; b < 8; ++b) {
        std::printf("%s-%s  %5d  %7d\n", clock(100.0 + 5 * b).substr(0, 5).c_str(),
                    clock(105.0 + 5 * b).substr(0, 5).c_str(), bucketTotal[b], bucketBricked[b]);
    }
    std::printf("devices: 1000, update interrupted by the power cut: %d\n", interrupted);
    std::printf("result: running v2 %d, still on v1 %d, BRICKED %d\n", onV2, onV1, bricked);
}

}  // namespace

int main(int argc, char** argv)
{
    const std::string which = argc > 1 ? argv[1] : "single";
    if (which == "single") {
        night<SingleSlot>("the fleet's design: single slot, erase then write, CRC check", true);
    } else {
        night<ABSlots>("replay of the same night: A/B slots + signature + test/confirm", true);
    }
    return 0;
}
