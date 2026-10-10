// Lab Engineer's checks for the KID101 final-exam answer key: every number in the key
// is printed here by a real run instead of being worked out only by hand. The methods
// are the ones of the chapters (F0-02 place values, F0-03 letter numbers printed by the
// toolchain, F0-04 network and "find the smallest", F0-05 toy kitchen, F0-06 limit,
// F0-07 paper adder, F0-09 card count).
#include <array>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

std::string eightSwitches(int number)
{
    std::string row;
    for (const int value : {128, 64, 32, 16, 8, 4, 2, 1}) {
        if (number >= value) { row += '1'; number = number - value; } else { row += '0'; }
    }
    return row;
}

int fromSwitches(const std::string& bits)
{
    int total = 0;
    int value = 1;
    for (std::size_t i = bits.size(); i > 0; --i) {
        if (bits[i - 1] == '1') { total = total + value; }
        value = value * 2;
    }
    return total;
}

std::string addByRules(const std::string& a, const std::string& b)
{
    std::string answer = "0000";
    int carry = 0;
    for (int column = 3; column >= 0; --column) {
        const int ones = (a[column] == '1') + (b[column] == '1') + carry;
        std::cout << "    column " << (column == 3 ? "1s" : column == 2 ? "2s" : column == 1 ? "4s" : "8s")
                  << ": bits " << a[column] << " and " << b[column] << ", carry in " << carry
                  << ", ones " << ones;
        if (ones == 0) { answer[column] = '0'; carry = 0; }
        if (ones == 1) { answer[column] = '1'; carry = 0; }
        if (ones == 2) { answer[column] = '0'; carry = 1; }
        if (ones == 3) { answer[column] = '1'; carry = 1; }
        std::cout << " -> write " << answer[column] << ", carry out " << carry << '\n';
    }
    return std::to_string(carry) + answer;
}

int main()
{
    std::cout << "F0-02: byte 01100100 = " << fromSwitches("01100100") << '\n';
    std::cout << "F0-02: 22 as eight switches = " << eightSwitches(22) << '\n';
    std::cout << "F0-02: six switches, all on, 111111 = " << fromSwitches("111111") << '\n';

    std::cout << "F0-03: decode 01000010 01011001 01000101 = ";
    for (const std::string bits : {"01000010", "01011001", "01000101"}) {
        const int number = fromSwitches(bits);
        std::cout << number << " (" << static_cast<char>(number) << ") ";
    }
    std::cout << '\n';

    std::cout << "F0-04: network trace for 4 2 3 1\n";
    std::array<int, 4> cards = {4, 2, 3, 1};
    const std::vector<std::pair<int, int>> comparators = {{0, 1}, {2, 3}, {0, 2}, {1, 3}, {1, 2}};
    int station = 0;
    for (const auto& [left, right] : comparators) {
        station = station + 1;
        if (cards[left] > cards[right]) { std::swap(cards[left], cards[right]); }
        std::cout << "    station " << station << " (spots " << left << " and " << right << "): "
                  << cards[0] << ' ' << cards[1] << ' ' << cards[2] << ' ' << cards[3] << '\n';
    }
    int comparisons = 0;
    for (int remaining = 7 - 1; remaining >= 1; --remaining) { comparisons = comparisons + remaining; }
    std::cout << "F0-04: find the smallest with 7 cards: 6 + 5 + 4 + 3 + 2 + 1 = " << comparisons << " comparisons\n";

    std::cout << "F0-05: toy kitchen with pantry 4, 9, 0 and the recipe LOAD_LEFT 0, LOAD_RIGHT 1, STORE_LEFT 2, ADD\n";
    std::vector<int> pantry = {4, 9, 0};
    int leftHand = 0;
    int rightHand = 0;
    for (const std::string step : {"LOAD_LEFT 0", "LOAD_RIGHT 1", "STORE_LEFT 2", "ADD"}) {
        if (step == "LOAD_LEFT 0") { leftHand = pantry[0]; }
        if (step == "LOAD_RIGHT 1") { rightHand = pantry[1]; }
        if (step == "STORE_LEFT 2") { pantry[2] = leftHand; }
        if (step == "ADD") { leftHand = leftHand + rightHand; }
        std::cout << "    " << step << " -> hands: " << leftHand << ", " << rightHand << "  pantry boxes: "
                  << pantry[0] << ", " << pantry[1] << ", " << pantry[2] << '\n';
    }

    std::cout << "F0-06: fan rule 'if the temperature is more than 25, fan ON'\n";
    for (const int reading : {20, 25, 26, 30}) {
        std::cout << "    reading " << reading << " -> " << (reading > 25 ? "fan ON" : "fan off") << '\n';
    }

    std::cout << "F0-07: paper computer 1011 + 0110\n";
    const std::string sum = addByRules("1011", "0110");
    std::cout << "    answer " << sum << " = " << fromSwitches(sum) << '\n';

    const int characters = 26;
    const int perCard = 8;
    const int totalCards = (characters + perCard - 1) / perCard;
    std::cout << "F0-09: " << characters << " characters on cards of " << perCard << " = " << totalCards
              << " cards (" << perCard << " x " << (totalCards - 1) << " = " << perCard * (totalCards - 1)
              << " is not enough; " << perCard << " x " << totalCards << " = " << perCard * totalCards << " is enough)\n";
    return 0;
}
