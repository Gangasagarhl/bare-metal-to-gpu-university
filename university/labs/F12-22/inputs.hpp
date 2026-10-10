// inputs.hpp - the characterization inputs, IN ORDER (the formatter remembers state).
// Chosen to touch every branch we could see in the code, plus the odd cases a log showed.
#include <string>
#include <vector>

inline const std::vector<std::string> kInputs = {
    "V=12.34;I=1.5;T=30",    // ordinary line
    "V=11.86;I=2.25;T=41.9", // two decimals in, one out; fractional temperature
    "I=0.5;T=35",            // no V field at all
    "V=10.49;I=-3.0;T=25",   // negative current, low voltage
    "V=12.6;I=0;T=60",       // temperature exactly at the edge
    "V=12.6;I=0;T=61",       // just above the edge
    "V=12.6",                // no current, no temperature
    "V=;I=1",                // empty value
    "X=1;V=9.99",            // unknown key first
    "V=12.6;T=-5",           // below zero
    "",                      // empty line
    "V=-0.5",                // negative voltage (a sensor fault)
};
