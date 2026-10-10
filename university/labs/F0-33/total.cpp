#include <iostream>

int main()
{
    int total = 0;
    for (int day = 1; day <= 10; ++day) {
        total = total + day;
        std::cout << "Day " << day << ": saved " << day << ", total " << total << '\n';
    }
    std::cout << "Saved in 10 days: " << total << '\n';
    return 0;
}
