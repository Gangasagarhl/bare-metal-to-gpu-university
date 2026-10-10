// flash_layout.cpp - F10-29 Listing 3: plan where the bootloader, the parameter storage and
// the application go in a microcontroller's flash, which is erased only in whole sectors.
// The sector sizes are those of the pretend U-MCU1 (exercise values); on a real port they
// come from the MCU's reference manual (flash memory organisation), cited in the dossier.
#include <cstdio>
#include <string>
#include <vector>

struct Region {
    std::string name;
    int firstSector;
    int sectors;
};

int main()
{
    // U-MCU1: 4 x 16 KiB, 1 x 64 KiB, 7 x 128 KiB (exercise layout, 1024 KiB in total)
    const std::vector<int> sectorKiB = {16, 16, 16, 16, 64, 128, 128, 128, 128, 128, 128, 128};
    const int bootloaderKiB = 14;   // size of our bootloader image (from its build)
    const int storageKiB = 32;      // parameter storage wanted: two sectors to alternate
    const int firmwareKiB = 850;    // size of the application image (from its build)

    std::vector<int> start(sectorKiB.size());
    int total = 0;
    for (std::size_t i = 0; i < sectorKiB.size(); ++i) {
        start[i] = total;
        total += sectorKiB[i];
    }
    std::printf("flash: %d KiB in %zu sectors\n", total, sectorKiB.size());

    // Bootloader: sector 0. Storage: the next sectors, in whole sectors. App: the rest.
    std::vector<Region> plan = {{"bootloader", 0, 1}, {"storage", 1, 2}, {"application", 3, 9}};
    bool ok = true;
    for (const Region& r : plan) {
        int kib = 0;
        for (int s = r.firstSector; s < r.firstSector + r.sectors; ++s) {
            kib += sectorKiB[static_cast<std::size_t>(s)];
        }
        std::printf("%-12s sectors %2d-%2d  offset %4d KiB  size %4d KiB\n", r.name.c_str(),
                    r.firstSector, r.firstSector + r.sectors - 1,
                    start[static_cast<std::size_t>(r.firstSector)], kib);
        const int need = r.name == "bootloader" ? bootloaderKiB
                         : r.name == "storage"  ? storageKiB
                                                : firmwareKiB;
        if (need > kib) {
            std::printf("  ERROR: %s needs %d KiB\n", r.name.c_str(), need);
            ok = false;
        } else {
            std::printf("  fits: needs %d KiB, %d KiB spare\n", need, kib - need);
        }
    }
    std::printf("application must be linked to start at offset %d KiB (0x%X)\n", start[3],
                start[3] * 1024);
    std::printf("layout %s\n", ok ? "OK" : "REJECTED");
    return ok ? 0 : 1;
}
