#include "units.h"

double celsius_to_fahrenheit(float celsius)
{
    return celsius * 9.0 / 5.0 + 32.0;
}

double grams_to_kilogram(double grams)
{
    return grams / 1000.0;
}

double litres_to_millilitres(double litres)
{
    print_banner();
    return litres * 1000.0;
}
