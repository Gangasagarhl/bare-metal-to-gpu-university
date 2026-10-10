// MA102 practical exam - REFERENCE SOLUTION (Lab Engineer; not given to candidates).
// This is status_exam_start.cpp with its four TODO lines completed.
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined status_exam.cpp -o decode
// Run:   ./decode < status_exam.in
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
        const unsigned zone = (status >> 4) & 0b111u;                     // TODO 1 done: bits 6..4
        const bool pump = (status & (1u << 3)) != 0;                       // TODO 2 done: bit 3
        const bool lid = (status & (1u << 2)) != 0;
        const unsigned level = status & 0b11u;                             // TODO 3 done: bits 1..0
        const int temperature = static_cast<std::int8_t>(tempByte);        // TODO 4 done: signed reading

        showByte("GSTAT", status);
        std::cout << ": ALARM " << alarm << ", ZONE " << zone << ", PUMP " << (pump ? "on" : "off")
                  << ", LID " << (lid ? "open" : "closed") << ", LEVEL " << level << " (" << levelNames[level & 3u] << ")\n";
        showByte("TEMP ", tempByte);
        std::cout << ": " << temperature << " C\n";
    }
    return 0;
}
