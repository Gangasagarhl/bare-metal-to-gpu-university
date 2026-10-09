// Listing 1 (F0-13): a fraction as a decimal and a percentage.
#include <iostream>

int main()
{
    double eaten = 3.0;
    double slices = 8.0;

    double part = eaten / slices;
    double percent = part * 100.0;

    std::cout << "fraction eaten: 3/8\n";
    std::cout << "as a decimal:   " << part << "\n";
    std::cout << "as a percent:   " << percent << " %\n";

    int wholeOnly = 3 / 8;
    std::cout << "3 / 8 with whole numbers only: " << wholeOnly << "\n";
    return 0;
}
