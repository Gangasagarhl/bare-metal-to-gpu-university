// Show 0 to 15 in decimal, binary and hexadecimal, then some whole bytes.
#include <bitset>
#include <iostream>

int main()
{
    std::cout << "dec  binary  hex\n";
    for (int n = 0; n <= 15; ++n) {
        std::cout << n << (n < 10 ? "    " : "   ")
                  << std::bitset<4>(n) << "    "
                  << std::hex << std::uppercase << n << std::dec << '\n';
    }
    std::cout << '\n';
    for (int byte : {0x2A, 0xC8, 0xFF}) {
        std::cout << std::bitset<8>(byte) << " = 0x" << std::hex << std::uppercase
                  << byte << std::dec << " = " << byte << '\n';
    }
    return 0;
}
