#include <iostream>

#include "units.h"

int main()
{
    print_banner();
    std::cout << "180 C = " << celsius_to_fahrenheit(180.0) << " F\n";
    std::cout << "250 g = " << grams_to_kilograms(250.0) << " kg\n";
    std::cout << "0.5 l = " << litres_to_millilitres(0.5) << " ml\n";
    return 0;
}
