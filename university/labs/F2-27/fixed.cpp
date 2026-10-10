// fixed.cpp: bugs.cc with every warning understood and fixed (not silenced).
#include <cstddef>
#include <cstdio>
#include <vector>

int grade(int score)
{
    if (score >= 50) {
        return 1;
    }
    return 0;                                 // fix 1: every path returns a value
}

double average(const std::vector<double>& xs)
{
    if (xs.empty()) {
        return 0.0;
    }
    double total = 0.0;                       // fix 2: accumulate in double
    for (double x : xs) {
        total += x;
    }
    return total / static_cast<double>(xs.size());
}

int count_negatives(const std::vector<int>& xs)
{
    int count = 0;                            // fix 3: initialised
    for (std::size_t i = 0; i < xs.size(); ++i) {   // index type matches size()
        if (xs[i] < 0) {
            ++count;
        }
    }
    return count;
}

int scale(int value)
{
    int factor = 2;
    if (value > 100) {
        factor = 10;                          // fix 4: assign the outer variable
    }
    return value * factor;
}

bool is_zero(int x)
{
    return x == 0;                            // fix 5: comparison
}

int main()
{
    std::printf("%d %g %d %d %d\n", grade(70), average({2.5, 2.5}),
                count_negatives({-1, 2, -3}), scale(200), is_zero(0));
    return 0;
}
