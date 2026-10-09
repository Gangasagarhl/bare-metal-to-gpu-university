// Fixed version for the forensic lab answer key.
#include <bitset>
#include <cstdint>
#include <iostream>

int main()
{
    std::uint8_t lights = 0b0000'1011;  // kitchen (0), hall (1) and bathroom (3) are on
    std::cout << "before: " << std::bitset<8>(lights) << '\n';
    lights = lights & ~(1u << 3);       // fixed: clear bit 3 only
    std::cout << "after:  " << std::bitset<8>(lights) << '\n';
    return 0;
}
