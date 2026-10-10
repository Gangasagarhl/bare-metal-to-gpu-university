// F0-50 Listing 3: how the work of C = A * B (all n x n) grows compared with its data.
#include <cstdint>
#include <iomanip>
#include <iostream>

int main()
{
    std::cout << "       n    multiply-adds n^3      numbers 3n^2   flops per number (2n/3)\n";
    for (std::uint64_t n = 2; n <= 4096; n *= 2) {
        const std::uint64_t madds = n * n * n;
        const std::uint64_t numbers = 3 * n * n;
        std::cout << std::setw(8) << n << std::setw(21) << madds << std::setw(18) << numbers
                  << std::setw(26) << std::fixed << std::setprecision(1)
                  << static_cast<double>(2 * madds) / static_cast<double>(numbers) << "\n";
    }
    return 0;
}
