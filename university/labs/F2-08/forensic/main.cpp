// main.cpp: reads lines such as "250 g kg" and prints the converted value.
#include <iostream>
#include <string>

#include "units.h"

int main()
{
    double value = 0.0;
    std::string from;
    std::string to;
    while (std::cin >> value >> from >> to) {
        double result = 0.0;
        if (convert(value, from, to, result)) {
            std::cout << value << ' ' << from << " = " << result << ' ' << to << '\n';
        } else {
            std::cout << "Sorry, I cannot convert " << from << " to " << to << '\n';
        }
    }
    return 0;
}
