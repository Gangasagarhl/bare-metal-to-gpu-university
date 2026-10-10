#include <algorithm>
#include <iostream>
#include <numeric>
#include <vector>

// Lab reference solution: kitchen statistics with standard algorithms only (no hand-written loops
// in the statistics functions).
int total_minutes(const std::vector<int>& m)
{
    return std::accumulate(m.begin(), m.end(), 0);
}

int longest(const std::vector<int>& m)
{
    return m.empty() ? 0 : *std::max_element(m.begin(), m.end());
}

bool over_twenty(int minutes)
{
    return minutes > 20;
}

long slow_orders(const std::vector<int>& m)
{
    return std::count_if(m.begin(), m.end(), over_twenty);
}

int median(std::vector<int> m)   // by value: we sort our own copy
{
    std::sort(m.begin(), m.end());
    return m.empty() ? 0 : m[m.size() / 2];
}

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

int main()
{
    std::vector<int> evening = {12, 25, 7, 30, 18, 9, 22};
    expect(total_minutes(evening) == 123, "total of the evening");
    expect(longest(evening) == 30, "longest order");
    expect(slow_orders(evening) == 3, "orders over 20 minutes");
    expect(median(evening) == 18, "median of 7 values");
    expect(evening.front() == 12, "median did not reorder the caller's vector");
    std::vector<int> none;
    expect(total_minutes(none) == 0 && longest(none) == 0, "empty evening");
    std::cout << "statistics tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
