#include <iostream>

int main()
{
    std::vector<int> scores = {7, 9, 4, 10, 6};
    int total = 0;
    int games = 0;
    int best = scores[0];
    for (int score : scores) {
        total = total + score;
        games = games + 1;
        if (score > best) {
            best = score;
        }
    }
    std::cout << "Games played: " << games << '\n';
    std::cout << "Total points: " << total << '\n';
    std::cout << "Best score: " << best << '\n';
    std::cout << "Average (whole number): " << total / games << '\n';
    return 0;
}
