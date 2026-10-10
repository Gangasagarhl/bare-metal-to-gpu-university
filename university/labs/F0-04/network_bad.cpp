#include <algorithm>
#include <array>
#include <iostream>
#include <utility>
#include <vector>

using Cards = std::array<int, 4>;

const std::vector<std::pair<int, int>> comparators = {{0, 1}, {2, 3}, {0, 2}, {1, 3}};

Cards runNetwork(Cards cards, bool show)
{
    for (const auto& [left, right] : comparators) {
        if (cards[left] > cards[right]) {
            std::swap(cards[left], cards[right]);
        }
        if (show) {
            std::cout << "compare spots " << left << " and " << right << ": ";
            for (const int card : cards) {
                std::cout << card << ' ';
            }
            std::cout << '\n';
        }
    }
    return cards;
}

int main()
{
    runNetwork({9, 4, 7, 2}, true);

    Cards order = {1, 2, 3, 4};
    int tried = 0;
    int sorted = 0;
    do {
        tried = tried + 1;
        const Cards result = runNetwork(order, false);
        if (std::is_sorted(result.begin(), result.end())) {
            sorted = sorted + 1;
        } else {
            std::cout << "NOT sorted: " << order[0] << ' ' << order[1] << ' ' << order[2]
                      << ' ' << order[3] << '\n';
        }
    } while (std::next_permutation(order.begin(), order.end()));
    std::cout << sorted << " of " << tried << " starting orders came out sorted.\n";
    return 0;
}
