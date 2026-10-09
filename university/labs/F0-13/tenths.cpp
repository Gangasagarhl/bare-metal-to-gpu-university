// Listing 2 (F0-13): a computer's decimals are close, not always exact.
#include <iomanip>
#include <iostream>

int main()
{
    double sum = 0.1 + 0.2;
    std::cout << "0.1 + 0.2 shown with 6 digits:  " << sum << "\n";
    std::cout << std::setprecision(17);
    std::cout << "0.1 + 0.2 shown with 17 digits: " << sum << "\n";
    std::cout << "1.0 / 3.0 shown with 17 digits: " << 1.0 / 3.0 << "\n";
    return 0;
}
