// overflow.cc: sums five readings, but the loop condition is off by one.
#include <cstdio>
#include <vector>

int sum_readings(const std::vector<int>& readings)
{
    int sum = 0;
    for (std::size_t i = 0; i <= readings.size(); ++i) {   // bug: <= reads one past the end
        sum += readings[i];
    }
    return sum;
}

int main()
{
    const std::vector<int> readings = {10, 20, 30, 40, 50};
    std::printf("sum = %d\n", sum_readings(readings));
    return 0;
}
