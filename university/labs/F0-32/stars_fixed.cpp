#include <iostream>

int main()
{
    int score = 0;
    std::cin >> score;
    std::cout << "Score: " << score << '\n';

    if (score >= 90) {
        std::cout << "Excellent: three stars!\n";
    } else if (score >= 50) {
        std::cout << "Good work: one star.\n";
    } else {
        std::cout << "Keep practising.\n";
    }
    return 0;
}
