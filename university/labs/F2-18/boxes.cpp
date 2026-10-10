#include <cstddef>
#include <cstdint>
#include <iostream>

// Memory as numbered boxes: how many bytes each type uses, and where four ints live.
int main()
{
    std::cout << "sizeof(char)      = " << sizeof(char) << '\n';
    std::cout << "sizeof(bool)      = " << sizeof(bool) << '\n';
    std::cout << "sizeof(short)     = " << sizeof(short) << '\n';
    std::cout << "sizeof(int)       = " << sizeof(int) << '\n';
    std::cout << "sizeof(long)      = " << sizeof(long) << '\n';
    std::cout << "sizeof(long long) = " << sizeof(long long) << '\n';
    std::cout << "sizeof(float)     = " << sizeof(float) << '\n';
    std::cout << "sizeof(double)    = " << sizeof(double) << '\n';
    std::cout << "sizeof(int*)      = " << sizeof(int*) << '\n';

    int shelf[4] = {10, 20, 30, 40};
    const auto first = reinterpret_cast<std::uintptr_t>(&shelf[0]);
    for (int i = 0; i < 4; ++i) {
        const auto here = reinterpret_cast<std::uintptr_t>(&shelf[i]);
        std::cout << "shelf[" << i << "] = " << shelf[i]
                  << "  at address 0x" << std::hex << here << std::dec
                  << "  (" << (here - first) << " bytes after shelf[0])\n";
    }
    std::cout << "sizeof(shelf)     = " << sizeof(shelf) << '\n';
    return 0;
}
