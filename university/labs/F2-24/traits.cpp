#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <string>
#include <type_traits>

struct Header                      // a plain record: safe to copy as bytes
{
    std::uint16_t kind;
    std::uint16_t length;
    std::uint32_t sequence;
};
static_assert(sizeof(Header) == 8, "Header must be exactly 8 bytes: a reader depends on it");
static_assert(offsetof(Header, sequence) == 4, "sequence must start at byte 4");

struct Named                       // owns a std::string: NOT a plain record
{
    int id;
    std::string name;
};

int main()
{
    std::cout << std::boolalpha;
    std::cout << "Header standard-layout:     " << std::is_standard_layout_v<Header> << '\n';
    std::cout << "Header trivially copyable:  " << std::is_trivially_copyable_v<Header> << '\n';
    std::cout << "Named  trivially copyable:  " << std::is_trivially_copyable_v<Named> << '\n';

    const Header h{2, 16, 1001};
    unsigned char wire[sizeof(Header)];
    std::memcpy(wire, &h, sizeof h);               // object -> bytes
    Header back{};
    std::memcpy(&back, wire, sizeof back);         // bytes -> object
    std::cout << "round trip: kind " << back.kind << ", length " << back.length
              << ", sequence " << back.sequence << '\n';
    return 0;
}
