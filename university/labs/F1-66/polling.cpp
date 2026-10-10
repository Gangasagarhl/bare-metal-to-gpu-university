// F1-66 forensic evidence generator: "The robot that thinks it went backwards".
// A wheel speeds up steadily for 2 s (synthetic motion, made here). A program polls
// the encoder's A/B lines every 0.5 ms (2000 times per second, pretend numbers) and
// decodes them with the table of Listing 1. The log compares its count with the
// true count of edges. The learner sees only the log; the cause is in the key.
#include <array>
#include <cmath>
#include <cstdio>

constexpr std::array<int, 16> kStep = {0, +1, -1, 2, -1, 0, 2, +1,
                                       +1, 2, 0, -1, 2, -1, +1, 0};
constexpr std::array<int, 4> kForward = {0b00, 0b01, 0b11, 0b10};

int main()
{
    const double accel = 4000.0;      // counts per second per second (synthetic)
    const double pollPeriod = 0.0005; // 2000 polls per second
    long decoded = 0;
    int invalid = 0;
    int prev = kForward[0];
    std::printf("%6s %12s %10s %10s %8s\n", "t s", "true cnt/s", "true cnt", "decoded", "invalid");
    for (int k = 0; k <= 4000; ++k) {
        const double t = k * pollPeriod;
        const long truePos = static_cast<long>(std::floor(0.5 * accel * t * t));
        const int ab = kForward[truePos % 4];
        const int step = kStep[(prev << 2) | ab];
        if (step == 2) {
            ++invalid;
        } else {
            decoded += step;
        }
        prev = ab;
        if (k % 200 == 0) {
            std::printf("%6.2f %12.0f %10ld %10ld %8d\n", t, accel * t, truePos, decoded, invalid);
        }
    }
    return 0;
}
