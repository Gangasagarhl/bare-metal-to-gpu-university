// test_units.cpp: unit tests for units.cpp. Build it together with units.cpp
// (not with main.cpp: each program has exactly one main).
#include <cassert>
#include <cmath>
#include <iostream>

#include "units.h"

// Two doubles count as equal when they differ by less than a tiny tolerance.
bool close_enough(double got, double expected)
{
    return std::abs(got - expected) < 1e-9;
}

void test_mass()
{
    assert(close_enough(grams_to_kilograms(250.0), 0.25));
    assert(close_enough(grams_to_kilograms(0.0), 0.0));
    assert(close_enough(kilograms_to_grams(1.5), 1500.0));
    assert(close_enough(kilograms_to_grams(grams_to_kilograms(123.0)), 123.0));
}

void test_volume()
{
    assert(close_enough(millilitres_to_litres(330.0), 0.33));
    assert(close_enough(litres_to_millilitres(0.5), 500.0));
}

void test_temperature()
{
    assert(close_enough(celsius_to_fahrenheit(0.0), 32.0));
    assert(close_enough(celsius_to_fahrenheit(100.0), 212.0));
    assert(close_enough(celsius_to_fahrenheit(-40.0), -40.0));
    assert(close_enough(fahrenheit_to_celsius(212.0), 100.0));
    assert(close_enough(fahrenheit_to_celsius(celsius_to_fahrenheit(37.5)), 37.5));
}

void test_convert()
{
    double result = 0.0;
    bool known = convert(250.0, "g", "kg", result);
    assert(known);
    assert(close_enough(result, 0.25));
    known = convert(2.0, "cups", "ml", result);
    assert(!known);
}

int main()
{
    test_mass();
    test_volume();
    test_temperature();
    test_convert();
    std::cout << "All unit tests passed.\n";
    return 0;
}
