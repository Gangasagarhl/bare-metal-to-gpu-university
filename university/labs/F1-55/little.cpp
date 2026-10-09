// F1-55 Listing 2: Little's law as a sizing tool.
// work in flight = latency x throughput. Reads lines "name latency rate" (any time unit,
// the same unit for both) and prints how much independent work must be in flight.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string name;
    double latency = 0.0;   // time one item takes from start to finish
    double rate = 0.0;      // items the machine can start per time unit
    std::printf("%-34s %10s %10s %12s\n", "case", "latency", "rate", "in flight");
    while (std::cin >> name >> latency >> rate) {
        double inFlight = latency * rate;
        std::printf("%-34s %10.2f %10.3f %12.0f\n", name.c_str(), latency, rate, std::ceil(inFlight));
    }
    return 0;
}
