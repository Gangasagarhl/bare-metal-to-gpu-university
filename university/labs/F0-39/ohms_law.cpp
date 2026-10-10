// Ohm's law with made-up exercise numbers (not taken from any real part).
#include <iostream>
#include <string>
#include <vector>

struct Loop
{
    std::string name;
    double volts;
    double ohms;
    bool closed;
};

int main()
{
    const std::vector<Loop> loops = {
        {"A", 6.0, 300.0, true},
        {"B", 6.0, 600.0, true},
        {"C", 3.0, 300.0, true},
        {"D", 6.0, 300.0, false},
    };
    for (const Loop& loop : loops) {
        double milliamps = 0.0;
        if (loop.closed) {
            milliamps = loop.volts * 1000.0 / loop.ohms;
        }
        std::cout << "Loop " << loop.name << ": " << loop.volts << " V, "
                  << loop.ohms << " ohms, " << (loop.closed ? "closed" : "open")
                  << " -> current " << milliamps << " mA\n";
    }
    return 0;
}
