#include <iostream>
#include <limits>

int main()
{
    std::cout << "bytes in a bool:   " << sizeof(bool) << '\n';
    std::cout << "bytes in a char:   " << sizeof(char) << '\n';
    std::cout << "bytes in an int:   " << sizeof(int) << '\n';
    std::cout << "bytes in a double: " << sizeof(double) << '\n';
    std::cout << "biggest int:   " << std::numeric_limits<int>::max() << '\n';
    std::cout << "smallest int: " << std::numeric_limits<int>::min() << '\n';
    return 0;
}
