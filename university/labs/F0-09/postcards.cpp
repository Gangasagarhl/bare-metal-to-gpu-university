#include <iostream>
#include <map>
#include <string>
#include <vector>

struct Postcard
{
    int number;
    int total;
    std::string words;
};

int main()
{
    const std::string message = "MEET ME AT THE BIG PARK GATE AT FOUR!";
    const int lettersPerCard = 8;
    const int total = (static_cast<int>(message.size()) + lettersPerCard - 1) / lettersPerCard;

    std::vector<Postcard> sent;
    for (int n = 1; n <= total; ++n) {
        sent.push_back({n, total, message.substr((n - 1) * lettersPerCard, lettersPerCard)});
    }
    std::cout << "sender wrote " << total << " postcards, each marked \"card n of " << total << "\"\n";

    // Our pretend network: cards arrive in this order, and card 2 is lost.
    const std::vector<int> arrivalOrder = {3, 1, 5, 4};

    std::map<int, std::string> received;
    int expected = 0;
    for (const int number : arrivalOrder) {
        const Postcard& card = sent.at(number - 1);
        std::cout << "arrived: card " << card.number << " of " << card.total << " \"" << card.words << "\"\n";
        received[card.number] = card.words;
        expected = card.total;
    }

    for (int n = 1; n <= expected; ++n) {
        if (received.count(n) == 0) {
            std::cout << "missing card " << n << ": receiver asks the sender to send it again\n";
            const Postcard& again = sent.at(n - 1);
            std::cout << "arrived: card " << again.number << " of " << again.total << " \"" << again.words << "\"\n";
            received[again.number] = again.words;
        }
    }

    std::string rebuilt;
    for (const auto& [number, words] : received) {
        rebuilt += words;
    }
    std::cout << "receiver put the cards in number order: \"" << rebuilt << "\"\n";
    return 0;
}
