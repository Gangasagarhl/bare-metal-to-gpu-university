#include <iostream>

int main()
{
    int orders = 0;
    std::cin >> orders;              // the input file holds 2147483646
    for (int day = 1; day <= 2; ++day) {
        orders = orders + 1;         // the second +1 goes past the largest int
        std::cout << "day " << day << ": orders = " << orders << '\n';
    }
    return 0;
}
