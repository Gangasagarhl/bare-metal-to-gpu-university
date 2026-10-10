#include <chrono>
#include <iostream>
#include <thread>

int main()
{
    std::cout << "Fill the bowl with 20 spoons of flour, 3 spoons at a time." << std::endl;
    int spoons = 0;
    while (spoons != 20) {
        spoons = spoons + 3;
        std::cout << "Spoons in the bowl: " << spoons << std::endl;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
    std::cout << "The bowl is full." << std::endl;
    return 0;
}
