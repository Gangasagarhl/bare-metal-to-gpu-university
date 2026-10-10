// MA102 practical exam - starting file. Complete the four TODO lines, then build and run.
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined status_exam_start.cpp -o decode
// Run:   ./decode < status_exam_start.in
//
// Each input line holds two bytes written in hex: the status byte GSTAT, then the soil temperature TEMP.
// GSTAT is the status register of the course's made-up greenhouse controller (invented for this exam;
// no real product uses it):
//   bit 7      ALARM  1 = alarm raised
//   bits 6..4  ZONE   the zone being watered, 0 to 7
//   bit 3      PUMP   1 = pump running
//   bit 2      LID    1 = roof lid open
//   bits 1..0  LEVEL  tank level: 0 = empty, 1 = low, 2 = half, 3 = full
// TEMP is the soil temperature in degrees Celsius as a SIGNED byte (two's complement), -40 to 60.
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
        const unsigned zone = 0;                        // TODO 1: ZONE is bits 6..4 (shift right, then mask) (F0-25)
        const bool pump = false;                        // TODO 2: PUMP is bit 3: test it with a mask (F0-25)
        const bool lid = (status & (1u << 2)) != 0;
        const unsigned level = 0;                       // TODO 3: LEVEL is bits 1..0 (mask only) (F0-25)
        const int temperature = tempByte;               // TODO 4: read TEMP as a signed byte, not unsigned (F0-24)

        showByte("GSTAT", status);
        std::cout << ": ALARM " << alarm << ", ZONE " << zone << ", PUMP " << (pump ? "on" : "off")
                  << ", LID " << (lid ? "open" : "closed") << ", LEVEL " << level << " (" << levelNames[level & 3u] << ")\n";
        showByte("TEMP ", tempByte);
        std::cout << ": " << temperature << " C\n";
    }
    return 0;
}
