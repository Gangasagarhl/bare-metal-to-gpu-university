// The same eight switches read two ways: unsigned (0 to 255) and signed (-128 to 127).
#include <bitset>
#include <cstdint>
#include <iostream>

void show(std::int8_t value)
{
    const auto pattern = static_cast<std::uint8_t>(value);
    std::cout << std::bitset<8>(pattern)
              << "  signed: " << static_cast<int>(value)
              << "  unsigned: " << static_cast<int>(pattern) << '\n';
}

int main()
{
    for (int n : {0, 1, 5, 127, -1, -5, -128}) {
        show(static_cast<std::int8_t>(n));
    }
    std::cout << '\n';

    const std::uint8_t five = 5;
    const auto flipped = static_cast<std::uint8_t>(~five);
    const auto plusOne = static_cast<std::uint8_t>(flipped + 1);
    std::cout << "5            " << std::bitset<8>(five) << '\n';
    std::cout << "flip all     " << std::bitset<8>(flipped) << '\n';
    std::cout << "add one      " << std::bitset<8>(plusOne)
              << "  read as signed: " << static_cast<int>(static_cast<std::int8_t>(plusOne))
              << '\n';
    std::cout << '\n';

    const std::uint8_t fromSensor = 200;
    const auto asSigned = static_cast<std::int8_t>(fromSensor);
    std::cout << "200 stored in a signed byte reads as " << static_cast<int>(asSigned) << '\n';
    const auto wrapped = static_cast<std::int8_t>(127 + 1);
    std::cout << "127 + 1 stored in a signed byte reads as " << static_cast<int>(wrapped) << '\n';
    return 0;
}
