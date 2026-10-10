#include <array>
#include <cstddef>
#include <iostream>

int main()
{
    std::array<int, 6> muffin_tin{};     // six cups, all set to 0
    muffin_tin[0] = 40;                  // grams of batter in cup 0
    muffin_tin[1] = 45;
    muffin_tin.at(5) = 50;

    int total = 0;
    for (std::size_t cup = 0; cup < muffin_tin.size(); ++cup) {
        std::cout << "cup " << cup << ": " << muffin_tin[cup] << " g\n";
        total += muffin_tin[cup];
    }
    std::cout << "The tin always has " << muffin_tin.size() << " cups; batter: "
              << total << " g\n";
    return 0;
}
