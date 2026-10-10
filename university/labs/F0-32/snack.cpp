#include <iostream>

int main()
{
    int hour = 0;
    bool homework_done = false;
    std::cin >> hour >> homework_done;

    bool after_school = hour >= 15;
    std::cout << std::boolalpha;
    std::cout << "hour: " << hour << ", homework done: " << homework_done << '\n';

    if (after_school && homework_done) {
        std::cout << "Yes, you may have a snack.\n";
    } else {
        std::cout << "Not yet.\n";
    }

    if (hour == 12 || hour == 18) {
        std::cout << "It is a mealtime.\n";
    }
    return 0;
}
