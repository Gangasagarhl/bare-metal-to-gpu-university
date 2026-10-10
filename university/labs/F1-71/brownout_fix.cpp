// F1-71: verification runs for the forensic key. Same model, three changes tried
// one at a time and then together; only the summary line of each run is printed.
#include "power_model.h"

#include <cstdio>

int main()
{
    struct Trial
    {
        const char* name;
        PowerConfig cfg;
    };
    const Trial trials[] = {
        {"as built", {false, 0.0, false}},
        {"MCU fed from pack terminals", {true, 0.0, false}},
        {"soft start 300 ms", {false, 300.0, false}},
        {"both changes", {true, 300.0, false}},
    };
    for (const Trial& t : trials) {
        std::printf("%-30s ", t.name);
        PowerConfig c = t.cfg;
        c.print = true;
        // run quietly but report: re-run with print off and count resets
        c.print = false;
        const int resets = simulatePower(c);
        std::printf("resets in 1.2 s: %d\n", resets);
    }
    return 0;
}
