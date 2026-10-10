#include <cstdint>
#include <iostream>

struct Dish
{
    int table;
    double price;
};

// A pointer is a number that says where an object is; * follows it, & takes it.
int main()
{
    int portions = 3;
    int* p = &portions;               // p holds the address of portions
    std::cout << "portions = " << portions << ", p = " << p << ", *p = " << *p << '\n';

    *p = 5;                           // write through the pointer
    std::cout << "after *p = 5: portions = " << portions << '\n';

    int shelf[4] = {10, 20, 30, 40};
    int* q = &shelf[0];
    q = q + 2;                        // moves by 2 ints, not by 2 bytes
    std::cout << "*q = " << *q << ", q - &shelf[0] = " << (q - &shelf[0]) << " elements, "
              << (reinterpret_cast<std::uintptr_t>(q) - reinterpret_cast<std::uintptr_t>(&shelf[0]))
              << " bytes\n";

    Dish soup{4, 2.5};
    Dish* d = &soup;
    d->price = 3.0;                   // same as (*d).price = 3.0
    std::cout << "soup: table " << soup.table << ", price " << soup.price << '\n';

    int* nothing = nullptr;           // points nowhere, on purpose
    if (nothing == nullptr) {
        std::cout << "nothing is a null pointer; never follow it\n";
    }
    std::cout << "sizeof(p) = " << sizeof(p) << ", sizeof(d) = " << sizeof(d) << '\n';
    return 0;
}
