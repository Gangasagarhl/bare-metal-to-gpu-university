// Forensic evidence: a blink loop with one line missing.
#include <iostream>

int main()
{
    const int onMs = 500;
    const int offMs = 500;
    const int blinks = 3;

    int clockMs = 0;
    for (int i = 0; i < blinks; ++i) {
        std::cout << "t=" << clockMs << " ms  LED ON\n";
        clockMs += onMs;
        clockMs += offMs;
    }
    std::cout << "simulated time: " << clockMs << " ms for " << blinks << " blinks\n";
    return 0;
}
