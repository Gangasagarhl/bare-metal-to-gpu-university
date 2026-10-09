#include <cstddef>
#include <iostream>
#include <limits>

int main()
{
    std::cout << "Sizes in bytes on this machine:\n";
    std::cout << "  char        " << sizeof(char) << '\n';
    std::cout << "  bool        " << sizeof(bool) << '\n';
    std::cout << "  int         " << sizeof(int) << '\n';
    std::cout << "  long long   " << sizeof(long long) << '\n';
    std::cout << "  double      " << sizeof(double) << '\n';
    std::cout << "  std::size_t " << sizeof(std::size_t) << '\n';
    std::cout << "Smallest int: " << std::numeric_limits<int>::min() << '\n';
    std::cout << "Largest int:  " << std::numeric_limits<int>::max() << '\n';
    std::cout << "Largest std::size_t: " << std::numeric_limits<std::size_t>::max() << '\n';
    return 0;
}
