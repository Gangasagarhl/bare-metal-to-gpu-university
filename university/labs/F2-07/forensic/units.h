#ifndef UNITS_H
#define UNITS_H

#include <iostream>

double celsius_to_fahrenheit(double celsius);
double grams_to_kilograms(double grams);
double litres_to_millilitres(double litres);

void print_banner()
{
    std::cout << "== Kitchen converter ==\n";
}

#endif
