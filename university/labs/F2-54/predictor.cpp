// predictor.cpp - two textbook branch predictors run on recorded outcome patterns.
// Input lines: <name> <pattern of T (taken) and N (not taken)>
#include <cstdio>
#include <iostream>
#include <string>

// Remembers only the last outcome and predicts it again.
int oneBitMisses(std::string const& p)
{
    bool last = true;  // start by predicting "taken"
    int misses = 0;
    for (char c : p) {
        bool const taken = (c == 'T');
        misses += (taken != last);
        last = taken;
    }
    return misses;
}

// Saturating counter 0..3: 0-1 predict not taken, 2-3 predict taken.
// One surprise moves it one step, so a single odd outcome does not flip a strong state.
int twoBitMisses(std::string const& p, std::string& trace)
{
    int counter = 2;  // start "weakly taken"
    int misses = 0;
    trace.clear();
    for (char c : p) {
        bool const taken = (c == 'T');
        bool const predictTaken = counter >= 2;
        misses += (taken != predictTaken);
        trace += static_cast<char>('0' + counter);
        if (taken && counter < 3) {
            ++counter;
        } else if (!taken && counter > 0) {
            --counter;
        }
    }
    return misses;
}

int main()
{
    std::string name;
    std::string pattern;
    std::printf("%-12s %6s %12s %12s\n", "pattern", "length", "1-bit misses", "2-bit misses");
    while (std::cin >> name >> pattern) {
        std::string trace;
        int const m1 = oneBitMisses(pattern);
        int const m2 = twoBitMisses(pattern, trace);
        std::printf("%-12s %6zu %12d %12d\n", name.c_str(), pattern.size(), m1, m2);
        if (pattern.size() <= 16) {
            std::printf("  outcomes        %s\n  2-bit counter   %s  (state before each outcome)\n",
                        pattern.c_str(), trace.c_str());
        }
    }
    return 0;
}
