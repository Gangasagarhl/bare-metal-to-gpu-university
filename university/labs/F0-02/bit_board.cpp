#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::vector<int> placeValues = {128, 64, 32, 16, 8, 4, 2, 1};
    std::string board;
    while (std::cin >> board) {
        if (board.size() != placeValues.size()) {
            std::cout << board << " has " << board.size() << " switches, not 8. Skipped.\n";
            continue;
        }
        int total = 0;
        std::cout << board << " = ";
        for (std::size_t i = 0; i < board.size(); ++i) {
            if (board[i] == '1') {
                std::cout << placeValues[i] << " ";
                total = total + placeValues[i];
            }
        }
        if (total == 0) {
            std::cout << "(no switch is on) ";
        }
        std::cout << "-> total " << total << '\n';
    }
    return 0;
}
