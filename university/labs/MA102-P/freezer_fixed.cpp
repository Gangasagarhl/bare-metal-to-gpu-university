// Fixed version of freezer_bug.cpp (MA102 final, forensic question F27): one changed line.
// The reading is now read as a signed byte (two's complement), as the freezer's description says.
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    const std::vector<std::uint8_t> packet = {0x04, 0x00, 0xFD, 0xF6, 0xEC, 0xE2};
    for (std::uint8_t byte : packet) {
        const int temperature = static_cast<std::int8_t>(byte);    // the fix: signed reading
        std::cout << "byte 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte) << std::dec << " -> freezer: " << temperature << " C\n";
    }
    return 0;
}
