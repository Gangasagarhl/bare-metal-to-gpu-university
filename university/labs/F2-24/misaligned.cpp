#include <cstdint>
#include <cstring>
#include <iostream>

// Reads a 4-byte number that starts at byte 1 of a buffer: first wrongly, then correctly.
int main()
{
    std::cout << std::unitbuf;
    alignas(4) unsigned char packet[8] = {0xAA, 0x2A, 0x00, 0x00, 0x00, 0xBB, 0xCC, 0xDD};

    std::uint32_t good = 0;
    std::memcpy(&good, packet + 1, sizeof good);                 // well-defined at any address
    std::cout << "memcpy read:  " << good << '\n';

    const auto* p = reinterpret_cast<const std::uint32_t*>(packet + 1);
    std::cout << "pointer read: " << *p << '\n';                 // misaligned AND type-punned: UB
    return 0;
}
