// F0-73 forensic evidence: an event counter kept in a float (the deliberate mistake is the type).
#include <cstdint>
#include <cstdio>

int main()
{
    float received = 0.0f;   // packets received, as the logger stored it
    std::uint64_t truth = 0; // what really happened
    std::printf("%-12s %-16s %-16s\n", "true count", "logged (float)", "difference");
    for (std::uint64_t i = 1; i <= 24000000; ++i) {
        received += 1.0f;
        truth = i;
        if (i % 4000000 == 0 || i == 16777215 || i == 16777216 || i == 16777217 || i == 16777218) {
            std::printf("%-12llu %-16.1f %-16.1f\n", static_cast<unsigned long long>(truth),
                        static_cast<double>(received),
                        static_cast<double>(truth) - static_cast<double>(received));
        }
    }
    return 0;
}
