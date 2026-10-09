// bugs.cc: five real bugs. The file compiles; which bugs are reported depends on the flags.
#include <cstdio>
#include <vector>

int grade(int score)
{
    if (score >= 50) {
        return 1;
    }
}                                             // bug 1: no return value when score < 50

double average(const std::vector<double>& xs)
{
    int total = 0;                            // bug 2: int cannot hold 2.5 exactly
    for (double x : xs) {
        total += x;
    }
    return total / xs.size();
}

int count_negatives(const std::vector<int>& xs)
{
    int count;                                // bug 3: never set to 0
    for (int i = 0; i < xs.size(); ++i) {     // (and an int compared with an unsigned size)
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
        int factor = 10;                      // bug 4: a new variable hides the outer one
        (void)factor;
    }
    return value * factor;
}

bool is_zero(int x)
{
    if (x = 0) {                              // bug 5: assignment, not comparison
        return true;
    }
    return false;
}

int main()
{
    std::printf("%d %g %d %d %d\n", grade(70), average({2.5, 2.5}),
                count_negatives({-1, 2, -3}), scale(200), is_zero(0));
    return 0;
}
