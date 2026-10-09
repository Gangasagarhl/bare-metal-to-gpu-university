// Listing 1 (F0-12): the four operations, the remainder, and rounding up.
#include <iostream>

int main()
{
    int a = 23;
    int b = 4;

    std::cout << "sum:        " << a + b << "\n";
    std::cout << "difference: " << a - b << "\n";
    std::cout << "product:    " << a * b << "\n";
    std::cout << "quotient:   " << a / b << "\n";
    std::cout << "remainder:  " << a % b << "\n";

    int cars = (a + b - 1) / b;
    std::cout << "cars needed for " << a << " children, " << b << " per car: " << cars << "\n";
    return 0;
}
