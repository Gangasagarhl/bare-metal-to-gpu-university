// A simulated microcontroller blink: a loop that turns an LED on, waits,
// turns it off, waits. Time is a pretend clock, so nothing really sleeps.
#include <iostream>

int main()
{
    const int onMs = 500;   // how long the LED stays on (exercise choice)
    const int offMs = 500;  // how long it stays off (exercise choice)
    const int blinks = 3;   // a real board would loop forever; we stop after 3

    int clockMs = 0;
    for (int i = 0; i < blinks; ++i) {
        std::cout << "t=" << clockMs << " ms  LED ON\n";
        clockMs += onMs;    // "wait onMs"
        std::cout << "t=" << clockMs << " ms  LED off\n";
        clockMs += offMs;   // "wait offMs"
    }
    std::cout << "simulated time: " << clockMs << " ms for " << blinks << " blinks\n";
    return 0;
}
