// streak.cc: the longest run of days on which the temperature rose.
// A learner reports: "for this week the answer should be 4, but it prints 2".
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
            if (run > best) {
                best = run;
            }
            run = 0;
        }
    }
    return best;
}

int main()
{
    const std::vector<int> week = {3, 4, 5, 2, 3, 4, 5, 6};
    std::printf("longest rise: %d\n", longest_rise(week));
    return 0;
}
