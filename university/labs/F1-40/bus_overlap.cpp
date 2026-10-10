// F1-40 forensic evidence: the same bus, but the decoder checks EVERY window and
// reports how many devices answered. One window in this map is wrong.
// When two devices drive the data lines at once, this model returns the AND of
// their bytes (like open-drain lines pulled low by either side) and flags it.
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Device
{
    std::string name;
    std::uint32_t base;
    std::uint32_t size;
    std::uint8_t value;   // every register of this toy device reads as this byte
};

int main()
{
    const std::vector<Device> map{
        {"RAM", 0x0000, 0x1000, 0x00},
        {"LEDS", 0x2000, 0x0004, 0x05},
        {"TIMER", 0x3000, 0x0100, 0x64},   // the board notes say: TIMER 0x3000-0x300F
        {"UART", 0x3080, 0x0008, 0x41},
    };
    std::printf("decoder table:\n");
    for (const Device& d : map) {
        std::printf("  %-5s base=0x%04X size=0x%04X  last=0x%04X\n", d.name.c_str(), d.base,
                    d.size, d.base + d.size - 1);
    }
    const std::uint32_t reads[] = {0x2000, 0x3004, 0x3080, 0x3081, 0x30F0};
    for (std::uint32_t a : reads) {
        int hits = 0;
        std::uint8_t bus = 0xFF;          // lines idle high
        std::string who;
        for (const Device& d : map) {
            if (a >= d.base && a - d.base < d.size) {
                ++hits;
                bus &= d.value;
                who += (who.empty() ? "" : "+") + d.name;
            }
        }
        std::printf("READ 0x%04X  selected=%-10s data=0x%02X%s\n", a, who.c_str(), bus,
                    hits > 1 ? "  <-- CONTENTION: more than one device drove the bus" : "");
    }
    return 0;
}
