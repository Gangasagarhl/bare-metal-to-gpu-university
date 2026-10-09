// forensic.cpp - F3-42 forensic evidence: the same battery read with the driver of
// firmware-support package 3.1 (its handshake is shown in the trace). Prints the battery
// percentage the user sees, every 30 s for 20 readings, and the trace of one odd reading.
#include "ec_model.h"

int main()
{
    ec::Controller controller(7);
    controller.setBattery(31200, 48000);
    for (int i = 0; i < 200; ++i) { controller.step(); }
    ec::HostDriver os(controller, false);
    std::printf("time   battery shown\n");
    bool shownTrace = false;
    std::vector<std::string> odd;
    for (int n = 0; n < 20; ++n) {
        os.log.clear();
        os.verbose = true;
        const uint16_t rem = os.read16(ec::kBatRemainingLo);
        os.verbose = false;
        const uint16_t full = os.read16(ec::kBatFullLo);
        const unsigned pct = full == 0 ? 0 : rem * 100u / full;
        std::printf("%3d s  %3u %%\n", n * 30, pct);
        if (pct != 65 && !shownTrace) { odd = os.log; shownTrace = true; }
        for (int i = 0; i < 300; ++i) { controller.step(); }
    }
    std::printf("--- driver trace of the first odd reading ---\n");
    for (const std::string& l : odd) { std::printf("%s\n", l.c_str()); }
    return 0;
}
