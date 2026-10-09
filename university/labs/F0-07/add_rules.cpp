#include <iostream>
#include <string>

// Adds two 4-switch numbers using only the written rules of the paper computer.
std::string addByRules(const std::string& a, const std::string& b)
{
    std::string answer = "0000";
    int carry = 0;
    for (int column = 3; column >= 0; --column) {
        const int ones = (a[column] == '1') + (b[column] == '1') + carry;
        if (ones == 0) { answer[column] = '0'; carry = 0; }
        if (ones == 1) { answer[column] = '1'; carry = 0; }
        if (ones == 2) { answer[column] = '0'; carry = 1; }
        if (ones == 3) { answer[column] = '1'; carry = 1; }
    }
    return std::to_string(carry) + answer;
}

std::string fourSwitches(int n)
{
    std::string s;
    for (const int value : {8, 4, 2, 1}) {
        if (n >= value) { s += '1'; n = n - value; } else { s += '0'; }
    }
    return s;
}

int main()
{
    std::cout << "0101 + 0011 = " << addByRules("0101", "0011") << '\n';

    int checked = 0;
    int wrong = 0;
    for (int x = 0; x <= 15; ++x) {
        for (int y = 0; y <= 15; ++y) {
            const std::string sum = addByRules(fourSwitches(x), fourSwitches(y));
            const int expected = x + y;
            const std::string expectedBits = (expected >= 16 ? "1" : "0") + fourSwitches(expected % 16);
            checked = checked + 1;
            if (sum != expectedBits) { wrong = wrong + 1; }
        }
    }
    std::cout << "checked " << checked << " sums, " << wrong << " wrong\n";
    return 0;
}
