#include <cstddef>
#include <iostream>

struct Loose          // members in an unlucky order
{
    char size;        // 'S', 'M' or 'L'
    double price;
    char spicy;       // 0 or 1
    int table;
};

struct Tidy           // the same members, largest alignment first
{
    double price;
    int table;
    char size;
    char spicy;
};

template <typename T>
void report(const char* name)
{
    std::cout << name << ": sizeof " << sizeof(T) << ", alignof " << alignof(T) << '\n';
}

int main()
{
    report<char>("char");
    report<int>("int");
    report<double>("double");
    report<Loose>("Loose");
    std::cout << "  offsets: size " << offsetof(Loose, size) << ", price " << offsetof(Loose, price)
              << ", spicy " << offsetof(Loose, spicy) << ", table " << offsetof(Loose, table) << '\n';
    report<Tidy>("Tidy");
    std::cout << "  offsets: price " << offsetof(Tidy, price) << ", table " << offsetof(Tidy, table)
              << ", size " << offsetof(Tidy, size) << ", spicy " << offsetof(Tidy, spicy) << '\n';
    std::cout << "array of 1000 Loose: " << sizeof(Loose[1000]) << " bytes; of 1000 Tidy: "
              << sizeof(Tidy[1000]) << " bytes\n";
    return 0;
}
