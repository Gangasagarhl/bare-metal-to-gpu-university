#include <cstdint>
#include <iostream>

// Checks the worked example: element k of an int array starting at 0x7ffc1000.
int main()
{
    const std::uint64_t start = 0x7ffc1000;
    const std::uint64_t intSize = 4;   // sizeof(int) printed by Listing 1 on this machine
    for (std::uint64_t k : {0u, 1u, 3u, 9u}) {
        const std::uint64_t address = start + k * intSize;
        std::cout << "element " << k << ": 0x" << std::hex << address << std::dec
                  << " = start + " << k * intSize << " bytes\n";
    }
    const std::uint64_t last = start + 99 * intSize;
    std::cout << "an array of 100 ints occupies 0x" << std::hex << start << " to 0x"
              << last + intSize - 1 << std::dec << ", " << 100 * intSize << " bytes\n";
    return 0;
}
