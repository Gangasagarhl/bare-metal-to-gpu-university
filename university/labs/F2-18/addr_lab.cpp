#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

// Lab: print the address and size of several objects, then explain the gaps.
template <typename T>
void show(const char* name, const T& object)
{
    std::cout << name << ": " << sizeof(object) << " bytes at 0x" << std::hex
              << reinterpret_cast<std::uintptr_t>(&object) << std::dec << '\n';
}

int main()
{
    const char letter = 'A';
    const int count = 7;
    const double price = 2.5;
    const std::string dish = "soup";
    const std::vector<int> orders = {4, 8, 15};

    show("letter", letter);
    show("count", count);
    show("price", price);
    show("dish (the std::string object)", dish);
    show("orders (the std::vector object)", orders);
    for (std::size_t i = 0; i < orders.size(); ++i) {
        std::cout << "orders[" << i << "] at 0x" << std::hex
                  << reinterpret_cast<std::uintptr_t>(&orders[i]) << std::dec << '\n';
    }
    return 0;
}
