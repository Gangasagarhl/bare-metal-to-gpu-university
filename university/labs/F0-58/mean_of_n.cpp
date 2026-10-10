// F0-58 Listing 2: the mean of n die rolls, repeated 4000 times for each n.
// How spread out are the means? Compare with sigma / sqrt(n).
#include <cmath>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

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
    const double sigma = std::sqrt(35.0 / 12.0);  // sd of one fair die
    std::mt19937 engine(58);
    const int repeats = 4000;
    std::cout << "one die: mean 3.5, standard deviation sigma = " << sigma << "\n";
    std::cout << "n     average of means   std dev of means   sigma/sqrt(n)\n";
    for (int n : {1, 4, 16, 64, 256}) {
        std::vector<double> means;
        for (int r = 0; r < repeats; ++r) {
            int total = 0;
            for (int i = 0; i < n; ++i) {
                total += rollDie(engine);
            }
            means.push_back(static_cast<double>(total) / n);
        }
        double sum = 0.0;
        for (double m : means) {
            sum += m;
        }
        const double average = sum / repeats;
        double ss = 0.0;
        for (double m : means) {
            ss += (m - average) * (m - average);
        }
        const double spread = std::sqrt(ss / (repeats - 1));
        std::cout << n << "\t" << average << "\t\t" << spread << "\t\t"
                  << sigma / std::sqrt(static_cast<double>(n)) << "\n";
    }
    return 0;
}
