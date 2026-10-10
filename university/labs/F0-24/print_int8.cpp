// A common surprise: printing a std::int8_t directly prints a character, not a number.
#include <cstdint>
#include <iostream>

int main()
{
    const std::int8_t small = 65;
    std::cout << "printed directly:   [" << small << "]\n";
    std::cout << "printed as an int:  [" << static_cast<int>(small) << "]\n";
    return 0;
}
