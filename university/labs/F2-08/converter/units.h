// units.h: the menu of the converter. Declarations only: names, parameter types and
// return types. The bodies (definitions) are in units.cpp.
#ifndef UNITS_H
#define UNITS_H

#include <string>

double grams_to_kilograms(double grams);
double kilograms_to_grams(double kilograms);
double millilitres_to_litres(double millilitres);
double litres_to_millilitres(double litres);
double celsius_to_fahrenheit(double celsius);
double fahrenheit_to_celsius(double fahrenheit);

// Converts value from the unit named `from` to the unit named `to`.
// Returns true and stores the answer in result, or returns false for an unknown pair.
bool convert(double value, const std::string& from, const std::string& to, double& result);

#endif
