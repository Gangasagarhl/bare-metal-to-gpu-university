// units.cpp: the definitions. It includes its own header, so the compiler can check
// that every definition matches its declaration.
#include "units.h"

double grams_to_kilograms(double grams)
{
    return grams / 1000.0;
}

double kilograms_to_grams(double kilograms)
{
    return kilograms * 1000.0;
}

double millilitres_to_litres(double millilitres)
{
    return millilitres / 1000.0;
}

double litres_to_millilitres(double litres)
{
    return litres * 1000.0;
}

double celsius_to_fahrenheit(double celsius)
{
    return celsius * 9.0 / 5.0 + 32.0;
}

double fahrenheit_to_celsius(double fahrenheit)
{
    return (fahrenheit - 32.0) * 5.0 / 9.0;
}

bool convert(double value, const std::string& from, const std::string& to, double& result)
{
    if (from == "g" && to == "kg") {
        result = grams_to_kilograms(value);
    } else if (from == "kg" && to == "g") {
        result = kilograms_to_grams(value);
    } else if (from == "ml" && to == "l") {
        result = millilitres_to_litres(value);
    } else if (from == "l" && to == "ml") {
        result = litres_to_millilitres(value);
    } else if (from == "C" && to == "F") {
        result = celsius_to_fahrenheit(value);
    } else if (from == "F" && to == "C") {
        result = fahrenheit_to_celsius(value);
    } else {
        return false;
    }
    return true;
}
