#include <iostream>

int main()
{
    int seconds = 0;
    while (seconds > 0) {
        std::cout << "Timer: " << seconds << '\n';
        seconds = seconds - 1;
    }
    std::cout << "Ding! The eggs are ready.\n";
    return 0;
}
