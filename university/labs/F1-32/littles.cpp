// Little's law for memory: bytes that must be "in flight" = bandwidth x latency.
// Each input line: a name, a latency in nanoseconds and a bandwidth in GB/s (1 GB = 10^9 bytes).
#include <iomanip>
#include <iostream>
#include <string>

int main()
{
    std::string name;
    double latencyNs = 0.0;
    double gigabytesPerSecond = 0.0;
    std::cout << std::fixed << std::setprecision(1);
    while (std::cin >> name >> latencyNs >> gigabytesPerSecond) {
        const double bytesPerNs = gigabytesPerSecond;           // 1 GB/s = 1 byte per ns
        const double bytesInFlight = bytesPerNs * latencyNs;    // Little's law
        const double linesInFlight = bytesInFlight / 64.0;      // 64-byte blocks
        const double oneAtATime = 64.0 / latencyNs;             // GB/s with one block in flight
        std::cout << name << ": " << latencyNs << " ns x " << gigabytesPerSecond << " GB/s = "
                  << bytesInFlight << " bytes in flight = " << linesInFlight
                  << " blocks of 64 B; one block at a time would give " << std::setprecision(2)
                  << oneAtATime << " GB/s\n" << std::setprecision(1);
    }
    return 0;
}
