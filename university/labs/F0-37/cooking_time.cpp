#include <iostream>
#include <vector>

int main()
{
    std::vector<int> minutes = {20, 35, 26};
    int total = 0;
    int days = 0;
    for (int m : minutes) {
        total = m;
        days = days + 1;
    }
    std::cout << "Days: " << days << '\n';
    std::cout << "Total minutes: " << total << '\n';
    std::cout << "Average minutes per day: " << total / days << '\n';
    return 0;
}
