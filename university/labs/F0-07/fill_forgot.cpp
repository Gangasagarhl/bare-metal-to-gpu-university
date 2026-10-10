#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    const int full = 4;
    int spoons = 0;
    std::cout << "start filling the cup" << std::endl;
    while (spoons < full) {
        std::cout << "still not full, cup has " << spoons << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
    std::cout << "cup is full" << std::endl;
    return 0;
}
