// Listing 1 (F0-11): build a number from its place-value parts.
#include <iostream>

int main()
{
    int thousands = 3;
    int hundreds = 4;
    int tens = 0;
    int ones = 7;

    int number = thousands * 1000 + hundreds * 100 + tens * 10 + ones;

    std::cout << "thousands part: " << thousands * 1000 << "\n";
    std::cout << "hundreds part:  " << hundreds * 100 << "\n";
    std::cout << "tens part:      " << tens * 10 << "\n";
    std::cout << "ones part:      " << ones << "\n";
    std::cout << "the number is:  " << number << "\n";
    return 0;
}
