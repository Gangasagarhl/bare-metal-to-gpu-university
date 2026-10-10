// F0-56 Listing 1: count every outcome of two dice and turn counts into probabilities.
#include <array>
#include <iostream>

int main()
{
    std::array<int, 13> ways{};  // ways[s] = number of outcomes whose sum is s (2..12)
    int total = 0;
    for (int red = 1; red <= 6; ++red) {
        for (int blue = 1; blue <= 6; ++blue) {
            ++ways[red + blue];
            ++total;
        }
    }
    std::cout << "outcomes in the sample space: " << total << "\n";
    std::cout << "sum  ways  probability\n";
    for (int s = 2; s <= 12; ++s) {
        const double p = static_cast<double>(ways[s]) / total;
        std::cout << (s < 10 ? " " : "") << s << "    " << ways[s] << "    "
                  << ways[s] << "/" << total << " = " << p << "\n";
    }
    int atLeastTen = ways[10] + ways[11] + ways[12];
    std::cout << "P(sum >= 10) = " << atLeastTen << "/" << total << " = "
              << static_cast<double>(atLeastTen) / total << "\n";
    std::cout << "P(sum <= 9)  = 1 - P(sum >= 10) = "
              << 1.0 - static_cast<double>(atLeastTen) / total << "\n";
    return 0;
}
