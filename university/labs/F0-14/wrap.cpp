// Listing 2 (F0-14): a whole-number type that has no negative numbers.
#include <iostream>

int main()
{
    int a = 5;
    int b = 7;
    std::cout << "int:      5 - 7 = " << a - b << "\n";

    unsigned int c = 5;
    unsigned int d = 7;
    std::cout << "unsigned: 5 - 7 = " << c - d << "\n";
    return 0;
}
