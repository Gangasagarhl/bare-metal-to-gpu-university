// F1-66 Listing 1: a quadrature decoder as a lookup table.
// Reads a sequence of A/B channel states (one "AB" pair of 0/1 per token) and
// counts +1 or -1 on every valid edge (x4 decoding). If both channels changed
// between two samples, the direction cannot be known: that is counted as an error.
#include <array>
#include <cstdio>
#include <iostream>
#include <string>

// Index = (previous AB << 2) | current AB, with AB = (A << 1) | B.
// Forward order of states: 00 -> 01 -> 11 -> 10 -> 00 (B leads A in this model).
// 0 = no change, +1 / -1 = one step, 2 = both channels changed (invalid).
constexpr std::array<int, 16> kStep = {
    0, +1, -1, 2,   // from 00 to 00, 01, 10, 11
    -1, 0, 2, +1,   // from 01
    +1, 2, 0, -1,   // from 10
    2, -1, +1, 0};  // from 11

int main()
{
    std::string token;
    int prev = -1;
    long count = 0;
    int errors = 0;
    int sample = 0;
    std::printf("%6s %3s %6s %6s\n", "sample", "AB", "step", "count");
    while (std::cin >> token) {
        if (token.size() != 2 || (token[0] != '0' && token[0] != '1')
            || (token[1] != '0' && token[1] != '1')) {
            std::printf("bad token '%s'\n", token.c_str());
            return 1;
        }
        const int ab = ((token[0] - '0') << 1) | (token[1] - '0');
        int step = 0;
        if (prev >= 0) {
            step = kStep[(prev << 2) | ab];
            if (step == 2) {
                ++errors;
            } else {
                count += step;
            }
        }
        std::printf("%6d %3s %6s %6ld\n", sample, token.c_str(),
                    step == 2 ? "ERROR" : (step > 0 ? "+1" : (step < 0 ? "-1" : "0")), count);
        prev = ab;
        ++sample;
    }
    std::printf("final count %ld, invalid transitions %d\n", count, errors);
    return 0;
}
