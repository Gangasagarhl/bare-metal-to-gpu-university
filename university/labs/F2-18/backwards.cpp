#include <array>
#include <cstdint>
#include <cstring>
#include <iomanip>
#include <iostream>

// Forensic evidence: one program saves a table number as raw bytes,
// another reads the bytes back assuming the highest byte comes first.
std::uint32_t readHighByteFirst(const std::array<unsigned char, 4>& b)
{
    return (std::uint32_t{b[0]} << 24) | (std::uint32_t{b[1]} << 16) |
           (std::uint32_t{b[2]} << 8) | std::uint32_t{b[3]};
}

int main()
{
    const std::uint32_t tableNumber = 258;
    std::array<unsigned char, 4> saved{};
    std::memcpy(saved.data(), &tableNumber, sizeof tableNumber);   // the writer

    std::cout << "writer stored table " << tableNumber << " as bytes:";
    for (const unsigned char b : saved) {
        std::cout << ' ' << std::hex << std::setw(2) << std::setfill('0')
                  << static_cast<unsigned>(b) << std::dec;
    }
    std::cout << '\n';
    std::cout << "reader decoded table " << readHighByteFirst(saved) << '\n';
    return 0;
}
