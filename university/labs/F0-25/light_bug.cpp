// Evidence program for the forensic lab: "the bathroom light will not turn off".
#include <bitset>
#include <cstdint>
#include <iostream>

int main()
{
    std::uint8_t lights = 0b0000'1011;  // kitchen (0), hall (1) and bathroom (3) are on
    std::cout << "before: " << std::bitset<8>(lights) << '\n';
    lights = lights & ~3u;              // meant: turn off the bathroom light (bit 3)
    std::cout << "after:  " << std::bitset<8>(lights) << '\n';
    return 0;
}
