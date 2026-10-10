// F0-56 Listing 2: roll two simulated dice many times; watch frequencies approach 6/36.
#include <cstdint>
#include <iostream>
#include <random>

// A die from the Mersenne Twister engine: rejection sampling keeps all six faces
// exactly equally likely (a plain "% 6" would favour some faces very slightly).
int rollDie(std::mt19937& engine)
{
    while (true) {
        const std::uint32_t r = engine();
        if (r < 4294967292u) {           // 4294967292 = 6 * 715827882, a multiple of 6
            return static_cast<int>(r % 6) + 1;
        }
    }
}

int main()
{
    std::mt19937 engine(2026);           // fixed seed: the same rolls on every run
    const long checkpoints[] = {36, 360, 3600, 36000, 360000};
    long rolls = 0;
    long sevens = 0;
    std::cout << "rolls     sevens   frequency   exact 6/36 = " << 6.0 / 36.0 << "\n";
    for (long target : checkpoints) {
        while (rolls < target) {
            if (rollDie(engine) + rollDie(engine) == 7) {
                ++sevens;
            }
            ++rolls;
        }
        std::cout << rolls << "\t  " << sevens << "\t   "
                  << static_cast<double>(sevens) / rolls << "\n";
    }
    return 0;
}
