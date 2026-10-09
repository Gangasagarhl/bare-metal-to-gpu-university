#include <iostream>
#include <random>

int pick_secret(unsigned int seed)
{
    std::mt19937 generator(seed);
    std::uniform_int_distribution<int> one_to_hundred(1, 100);
    return one_to_hundred(generator);
}

int main()
{
    std::random_device device;
    std::cout << "Seed 2026 twice: " << pick_secret(2026) << ' ' << pick_secret(2026) << '\n';
    std::cout << "Five seeds from std::random_device:";
    for (int round = 1; round <= 5; ++round) {
        std::cout << ' ' << pick_secret(device());
    }
    std::cout << '\n';
    return 0;
}
