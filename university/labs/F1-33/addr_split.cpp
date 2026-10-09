// Split addresses into tag, set index and block offset for a cache geometry.
// Input: first line "sets lineBytes", then one hexadecimal address per line.
#include <bitset>
#include <cstdint>
#include <iostream>
#include <string>

unsigned log2u(unsigned x)
{
    unsigned n = 0;
    while (x > 1) {
        x >>= 1;
        ++n;
    }
    return n;
}

int main()
{
    unsigned sets = 0;
    unsigned lineBytes = 0;
    std::cin >> sets >> lineBytes;
    const unsigned offsetBits = log2u(lineBytes);
    const unsigned indexBits = log2u(sets);
    std::cout << "offset bits " << offsetBits << ", index bits " << indexBits
              << ", tag = the remaining high bits\n";
    std::string word;
    while (std::cin >> word) {
        const std::uint64_t a = std::stoull(word, nullptr, 16);
        const std::uint64_t offset = a & (lineBytes - 1);
        const std::uint64_t index = (a >> offsetBits) & (sets - 1);
        const std::uint64_t tag = a >> (offsetBits + indexBits);
        std::cout << "0x" << std::hex << a << " = " << std::bitset<20>(a) << "  tag 0x" << tag
                  << "  set " << std::dec << index << "  offset " << offset << '\n';
    }
    return 0;
}
