#include <array>
#include <bit>
#include <cstdint>
#include <iomanip>
#include <iostream>

// Look inside one 4-byte number: which byte sits in the lowest-numbered box?
int main()
{
    const std::uint32_t value = 0x11223344;
    const auto bytes = std::bit_cast<std::array<unsigned char, 4>>(value);

    std::cout << "value = 0x" << std::hex << value << '\n';
    for (std::size_t i = 0; i < bytes.size(); ++i) {
        std::cout << "byte at offset " << i << " = 0x" << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned>(bytes[i]) << '\n';
    }
    if constexpr (std::endian::native == std::endian::little) {
        std::cout << "this machine is little-endian: lowest address holds the lowest byte\n";
    } else if constexpr (std::endian::native == std::endian::big) {
        std::cout << "this machine is big-endian: lowest address holds the highest byte\n";
    } else {
        std::cout << "this machine has a mixed byte order\n";
    }
    return 0;
}
