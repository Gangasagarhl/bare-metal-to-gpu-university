// forensic.cpp - F11-01 forensic evidence generator: "The robot that booted anyway".
// Two production batches of the same robot; a technician read each unit's flash with a debug
// probe and computed the stage digests. This program prints the evidence pack: the vendor's
// build records, the units' flash digests and whether each unit booted.
#include "chain_model.h"

namespace {

struct Unit {
    std::string id;
    int batch;
    std::vector<Stage> flash;
    Digest rom;
};

}  // namespace

int main()
{
    // build records: batch 1 and batch 2 were built from the same sources, on two build machines
    const std::vector<Stage> b1 = build_release("app v1: drive the robot", true);
    const std::vector<Stage> b2 = build_release("app v1: drive the robot", false);
    std::cout << "EVIDENCE 1 - build records (digest of each stage as built; ROM holds the bl1 digest)\n";
    std::cout << "  batch  bl1       bl2       os        app       ROM value  build flags of bl2\n";
    const std::vector<std::vector<Stage>> builds = {b1, b2};
    for (int b = 0; b < 2; ++b) {
        std::cout << "  " << b + 1 << "      ";
        for (const Stage& s : builds[b]) {
            std::cout << short_hex(digest_of(s)) << "  ";
        }
        std::cout << short_hex(digest_of(builds[b][0])) << "   "
                  << (b == 0 ? "CONFIG_VERIFY_NEXT=y" : "(flags not recorded)") << "\n";
    }

    std::vector<Unit> units = {{"R-0107", 1, b1, digest_of(b1[0])},
                               {"R-0112", 1, b1, digest_of(b1[0])},
                               {"R-0391", 2, b2, digest_of(b2[0])},
                               {"R-0398", 2, b2, digest_of(b2[0])}};
    units[1].flash[2].code = "os v1 with a hidden remote shell";   // same attack on both units
    units[3].flash[2].code = "os v1 with a hidden remote shell";

    std::cout << "\nEVIDENCE 2 - flash read-out of four units (debug probe), and boot result\n";
    std::cout << "  unit    batch  bl1       bl2       os        app       boots?\n";
    for (const Unit& u : units) {
        std::cout << "  " << u.id << "  " << u.batch << "      ";
        for (const Stage& s : u.flash) {
            std::cout << short_hex(digest_of(s)) << "  ";
        }
        std::cout << (boot(u.rom, u.flash, false) ? "yes" : "no (stops in boot)") << "\n";
    }
    std::cout << "\nEVIDENCE 3 - field report: R-0398 opened a network connection nobody configured.\n";
    return 0;
}
