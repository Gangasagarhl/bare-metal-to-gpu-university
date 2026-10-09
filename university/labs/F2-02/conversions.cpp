#include <cmath>
#include <iomanip>
#include <iostream>

int main()
{
    // Whole numbers divide into whole numbers: the remainder is thrown away.
    std::cout << "7 / 2   = " << 7 / 2 << '\n';
    std::cout << "7 % 2   = " << 7 % 2 << '\n';
    std::cout << "7.0 / 2 = " << 7.0 / 2 << '\n';

    // double to int cuts off the fraction (towards zero); it does not round.
    double soup_litres = 2.9;
    int whole_litres = static_cast<int>(soup_litres);
    std::cout << "static_cast<int>(2.9)  = " << whole_litres << '\n';
    std::cout << "static_cast<int>(-2.9) = " << static_cast<int>(-2.9) << '\n';
    std::cout << "std::lround(2.9)       = " << std::lround(soup_litres) << '\n';

    // A char is a small whole number that is printed as a letter.
    char letter = 'A';
    std::cout << "'A' as int  = " << static_cast<int>(letter) << '\n';
    std::cout << "66 as char  = " << static_cast<char>(66) << '\n';

    // Any non-zero number becomes true.
    std::cout << std::boolalpha;
    std::cout << "static_cast<bool>(5) = " << static_cast<bool>(5) << '\n';
    std::cout << "static_cast<bool>(0) = " << static_cast<bool>(0) << '\n';

    // Most decimal fractions are not stored exactly.
    double sum = 0.1 + 0.2;
    std::cout << std::setprecision(17);
    std::cout << "0.1 + 0.2 = " << sum << '\n';
    std::cout << "0.1 + 0.2 == 0.3 is " << (sum == 0.3) << '\n';
    return 0;
}
