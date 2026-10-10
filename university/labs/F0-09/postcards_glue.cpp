#include <iostream>
#include <string>
#include <vector>

int main()
{
    const std::string message = "MEET ME AT THE BIG PARK GATE AT FOUR!";
    const int lettersPerCard = 8;

    std::vector<std::string> cards;
    for (std::size_t start = 0; start < message.size(); start += lettersPerCard) {
        cards.push_back(message.substr(start, lettersPerCard));
    }
    std::cout << "sender wrote " << cards.size() << " postcards (no numbers on them)\n";

    // The same pretend network: cards arrive in this order, and the second card is lost.
    const std::vector<int> arrivalOrder = {3, 1, 5, 4};

    std::string rebuilt;
    for (const int position : arrivalOrder) {
        std::cout << "arrived: \"" << cards.at(position - 1) << "\"\n";
        rebuilt += cards.at(position - 1);
    }
    std::cout << "receiver glued the cards in arrival order: \"" << rebuilt << "\"\n";
    return 0;
}
