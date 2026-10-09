#include <cstddef>
#include <cstring>
#include <iomanip>
#include <iostream>

struct Loose
{
    char size;
    double price;
    char spicy;
    int table;
};

// Shows every byte of one object, including the padding bytes the program never set.
int main()
{
    Loose order{};                       // value-initialised: members AND padding start as zero
    order.size = 'M';
    order.price = 2.5;
    order.spicy = 1;
    order.table = 7;
    unsigned char bytes[sizeof(Loose)];
    std::memcpy(bytes, &order, sizeof order);   // copy the object representation
    for (std::size_t i = 0; i < sizeof bytes; ++i) {
        std::cout << std::setw(2) << std::setfill('0') << std::hex
                  << static_cast<unsigned>(bytes[i]) << (i % 8 == 7 ? '\n' : ' ');
    }
    std::cout << std::dec << "(" << sizeof bytes << " bytes)\n";
    return 0;
}
