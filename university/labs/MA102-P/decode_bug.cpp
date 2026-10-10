// MA102 practical exam, Part C evidence (Lab Engineer; candidates get only decode_bug.out).
// This is the reference solution with ONE deliberate mistake: the ZONE mask is four bits wide
// instead of three, so the ALARM bit (bit 7) leaks into the zone number.
#include <bitset>
#include <cstdint>
#include <iomanip>
#include <iostream>

void showByte(const char* name, std::uint8_t byte)
{
    std::cout << name << ' ' << std::bitset<8>(byte) << " (0x" << std::hex << std::uppercase << std::setw(2)
              << std::setfill('0') << static_cast<int>(byte) << std::dec << ")";
}

int main()
{
    const char* levelNames[4] = {"empty", "low", "half", "full"};
    unsigned statusIn = 0;
    unsigned tempIn = 0;
    while (std::cin >> std::hex >> statusIn >> tempIn) {
        const std::uint8_t status = static_cast<std::uint8_t>(statusIn);
        const std::uint8_t tempByte = static_cast<std::uint8_t>(tempIn);

        const unsigned alarm = (status >> 7) & 1u;
        const unsigned zone = (status >> 4) & 0b1111u;                    // the deliberate mistake (mask too wide)
        const bool pump = (status & (1u << 3)) != 0;
        const bool lid = (status & (1u << 2)) != 0;
        const unsigned level = status & 0b11u;
        const int temperature = static_cast<std::int8_t>(tempByte);

        showByte("GSTAT", status);
        std::cout << ": ALARM " << alarm << ", ZONE " << zone << ", PUMP " << (pump ? "on" : "off")
                  << ", LID " << (lid ? "open" : "closed") << ", LEVEL " << level << " (" << levelNames[level & 3u] << ")\n";
        showByte("TEMP ", tempByte);
        std::cout << ": " << temperature << " C\n";
    }
    return 0;
}
