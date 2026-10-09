// F0-56 forensic evidence: Leo's "chance of at least one six in k rolls" table,
// next to what 100000 simulated games per row really show. One rule is misused.
#include <cstdint>
#include <iostream>
#include <random>

int rollDie(std::mt19937& engine)
{
    while (true) {
        const std::uint32_t r = engine();
        if (r < 4294967292u) {
            return static_cast<int>(r % 6) + 1;
        }
    }
}

int main()
{
    std::mt19937 engine(56);
    const int games = 100000;
    std::cout << "k rolls | Leo's formula | games with a six (of " << games << ") | frequency\n";
    for (int k = 1; k <= 8; ++k) {
        double leo = 0.0;
        for (int i = 0; i < k; ++i) {
            leo += 1.0 / 6.0;  // Leo adds the chance of a six once per roll
        }
        int withSix = 0;
        for (int g = 0; g < games; ++g) {
            bool sawSix = false;
            for (int i = 0; i < k; ++i) {
                if (rollDie(engine) == 6) {
                    sawSix = true;
                }
            }
            if (sawSix) {
                ++withSix;
            }
        }
        std::cout << "   " << k << "    |   " << leo << "   |   " << withSix << "   |   "
                  << static_cast<double>(withSix) / games << "\n";
    }
    return 0;
}
