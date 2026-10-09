#include <cstddef>
#include <iostream>

struct Mixed
{
    char size_code;                      // 'S', 'M' or 'L'
    double price;
    char spice_level;                    // '0' to '3'
};

struct Reordered
{
    double price;
    char size_code;
    char spice_level;
};

int main()
{
    std::cout << "sizeof(char) + sizeof(double) + sizeof(char) = "
              << sizeof(char) + sizeof(double) + sizeof(char) << '\n';
    std::cout << "sizeof(Mixed)     = " << sizeof(Mixed) << '\n';
    std::cout << "  offsetof size_code   = " << offsetof(Mixed, size_code) << '\n';
    std::cout << "  offsetof price       = " << offsetof(Mixed, price) << '\n';
    std::cout << "  offsetof spice_level = " << offsetof(Mixed, spice_level) << '\n';
    std::cout << "sizeof(Reordered) = " << sizeof(Reordered) << '\n';
    std::cout << "  offsetof price       = " << offsetof(Reordered, price) << '\n';
    std::cout << "  offsetof size_code   = " << offsetof(Reordered, size_code) << '\n';
    std::cout << "  offsetof spice_level = " << offsetof(Reordered, spice_level) << '\n';
    std::cout << "alignof(double) = " << alignof(double) << '\n';
    return 0;
}
