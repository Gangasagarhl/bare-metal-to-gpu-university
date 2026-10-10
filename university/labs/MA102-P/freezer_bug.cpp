// Evidence program for the MA102 final exam, forensic question F27: "the freezer at 253 degrees".
// The pretend freezer sends one SIGNED byte per reading: its temperature in degrees Celsius, -40 to 40.
// The bytes below are the ones that arrived (made up for the exam). The program contains one deliberate mistake.
#include <cstdint>
#include <iomanip>
#include <iostream>
#include <vector>

int main()
{
    const std::vector<std::uint8_t> packet = {0x04, 0x00, 0xFD, 0xF6, 0xEC, 0xE2};
    for (std::uint8_t byte : packet) {
        const unsigned temperature = byte;    // the reading, as the program stores it
        std::cout << "byte 0x" << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
                  << static_cast<int>(byte) << std::dec << " -> freezer: " << temperature << " C\n";
    }
    return 0;
}
