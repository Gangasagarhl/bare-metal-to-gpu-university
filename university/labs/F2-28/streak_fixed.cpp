// streak_fixed.cpp: streak.cc after the debugging session. The run that is still going
// when the loop ends is now counted too.
#include <cstddef>
#include <cstdio>
#include <vector>

int longest_rise(const std::vector<int>& temps)
{
    int best = 0;
    int run = 0;
    for (std::size_t i = 1; i < temps.size(); ++i) {
        if (temps[i] > temps[i - 1]) {
            ++run;
        } else {
            run = 0;
        }
        if (run > best) {                     // compare after every step, not only at a fall
            best = run;
        }
    }
    return best;
}

int main()
{
    const std::vector<int> week = {3, 4, 5, 2, 3, 4, 5, 6};
    const std::vector<int> falling = {9, 8, 7};
    const std::vector<int> empty = {};
    std::printf("longest rise: %d %d %d\n", longest_rise(week), longest_rise(falling),
                longest_rise(empty));
    return longest_rise(week) == 4 ? 0 : 1;
}
