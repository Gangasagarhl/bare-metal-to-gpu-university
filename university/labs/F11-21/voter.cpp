// voter.cpp - F11-21: redundancy against random and systematic faults.
// Three channels compute a joint's speed (counts per ms) from a 16-bit encoder counter.
// Implementation X has a systematic bug: it ignores the counter's wrap from 65535 to 0.
// Implementations Y and Z are correct and written differently (diverse).
// Fault types injected:
//   random     : one bit of channel 2's copy of the counter flips at one step
//   systematic : the counter wraps (it does in every scenario, at step 42)
// Architectures: 2oo3 with mid-value select (output = median of three), 1oo2 comparison
// (two channels; any disagreement -> safe state).
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

using Impl = int (*)(std::uint16_t now, std::uint16_t prev);

int impl_x(std::uint16_t now, std::uint16_t prev)        // BUG: no wrap handling
{
    return static_cast<int>(now) - static_cast<int>(prev);
}

int impl_y(std::uint16_t now, std::uint16_t prev)        // modular difference (C++20: defined)
{
    return static_cast<std::int16_t>(static_cast<std::uint16_t>(now - prev));
}

int impl_z(std::uint16_t now, std::uint16_t prev)        // explicit wrap test
{
    int d = static_cast<int>(now) - static_cast<int>(prev);
    if (d < -32768) {
        d += 65536;
    } else if (d > 32767) {
        d -= 65536;
    }
    return d;
}

struct Scenario {
    std::string name;
    std::vector<Impl> channels;   // 2 channels: 1oo2 comparison, 3 channels: 2oo3 voting
    int flip_step;                // step of a random bit flip in channel 2 (-1 = none)
};

void run(const Scenario& s)
{
    const int speed = 37;                       // true counts per ms
    std::uint16_t count = 64000;                // wraps between step 41 and 42
    std::uint16_t prev = count;
    int wrong = 0, detected = 0, first_wrong = -1, first_detect = -1;
    bool safe_state = false;
    for (int step = 1; step <= 200 && !safe_state; ++step) {
        count = static_cast<std::uint16_t>(count + speed);
        std::vector<int> out;
        for (std::size_t c = 0; c < s.channels.size(); ++c) {
            std::uint16_t seen = count;
            if (c == 1 && step == s.flip_step) {
                seen = static_cast<std::uint16_t>(seen ^ 0x0400);   // random hardware fault
            }
            out.push_back(s.channels[c](seen, prev));
        }
        prev = count;
        int result = 0;
        bool disagree = false;
        if (out.size() == 3) {
            std::vector<int> sorted = out;
            std::sort(sorted.begin(), sorted.end());
            result = sorted[1];                                    // mid-value select
            disagree = (sorted[2] - sorted[0]) > 2;
        } else {
            result = out[0];
            disagree = std::abs(out[0] - out[1]) > 2;
            safe_state = disagree;                                 // 1oo2: stop on mismatch
        }
        if (disagree) {
            ++detected;
            first_detect = first_detect < 0 ? step : first_detect;
        }
        if (!safe_state && std::abs(result - speed) > 2) {
            ++wrong;
            first_wrong = first_wrong < 0 ? step : first_wrong;
        }
    }
    std::printf("%-34s wrong outputs %d (first at step %3d) | disagreements %d (first %3d) | %s\n",
                s.name.c_str(), wrong, first_wrong, detected, first_detect,
                safe_state ? "SAFE STATE" : (wrong > 0 ? "WRONG VALUE USED" : "ok"));
}

int main()
{
    const std::vector<Scenario> all = {
        {"2oo3 identical X,X,X  flip + wrap", {impl_x, impl_x, impl_x}, 100},
        {"2oo3 identical X,X,X  wrap only", {impl_x, impl_x, impl_x}, -1},
        {"2oo3 diverse X,Y,Z    wrap only", {impl_x, impl_y, impl_z}, -1},
        {"2oo3 diverse X,Y,Z    flip + wrap", {impl_x, impl_y, impl_z}, 100},
        {"1oo2 identical Y,Y    flip + wrap", {impl_y, impl_y}, 100},
        {"1oo2 diverse X,Y      wrap only", {impl_x, impl_y}, -1},
    };
    std::puts("true speed 37 counts/ms; the 16-bit counter wraps at step 42; flips at step 100");
    for (const auto& s : all) {
        run(s);
    }
    return 0;
}
