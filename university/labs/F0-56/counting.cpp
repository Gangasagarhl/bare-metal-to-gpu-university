// F0-56 Listing 3: counting rules (factorial, arrangements, choices) and "at least one".
#include <cmath>
#include <cstdint>
#include <iostream>

std::uint64_t factorial(int n)
{
    std::uint64_t result = 1;
    for (int i = 2; i <= n; ++i) {
        result *= static_cast<std::uint64_t>(i);
    }
    return result;
}

// Ordered arrangements of k items chosen from n: n * (n-1) * ... * (n-k+1).
std::uint64_t arrangements(int n, int k)
{
    std::uint64_t result = 1;
    for (int i = 0; i < k; ++i) {
        result *= static_cast<std::uint64_t>(n - i);
    }
    return result;
}

// Unordered choices of k items from n ("n choose k"), built step by step so that
// every intermediate value is itself a whole number.
std::uint64_t choose(int n, int k)
{
    std::uint64_t result = 1;
    for (int i = 1; i <= k; ++i) {
        result = result * static_cast<std::uint64_t>(n - k + i) / static_cast<std::uint64_t>(i);
    }
    return result;
}

int main()
{
    std::cout << "ways to line up 4 cups on a shelf: 4! = " << factorial(4) << "\n";
    std::cout << "ordered podium (1st, 2nd, 3rd) from 8 runners: " << arrangements(8, 3) << "\n";
    std::cout << "unordered teams of 3 from 8 friends: C(8,3) = " << choose(8, 3) << "\n";
    std::cout << "4-digit door codes, digits 0-9, repeats allowed: " << 10 * 10 * 10 * 10 << "\n";
    std::cout << "P(one given team of 3 is picked at random) = 1/" << choose(8, 3) << " = "
              << 1.0 / static_cast<double>(choose(8, 3)) << "\n";

    // "At least one": the complement of "none". Exercise numbers, not hardware data.
    const double pFailOneDay = 0.01;
    for (int machines : {1, 10, 100, 500}) {
        const double pNone = std::pow(1.0 - pFailOneDay, machines);
        std::cout << "machines " << machines << ": P(at least one fails today) = "
                  << 1.0 - pNone << "\n";
    }
    return 0;
}
