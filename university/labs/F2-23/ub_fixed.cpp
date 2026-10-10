#include <algorithm>
#include <climits>
#include <iostream>
#include <optional>
#include <string>

// Lab answer: the three programs of the lab, each with one possible fix.
std::optional<int> addPortions(int a, int b)        // overflow: detect it instead of doing it
{
    if ((b > 0 && a > INT_MAX - b) || (b < 0 && a < INT_MIN - b)) {
        return std::nullopt;
    }
    return a + b;
}

std::string makeLabel(int table)
{
    return "table " + std::to_string(table) + ", window seat";
}

int main()
{
    const auto sum = addPortions(INT_MAX, 1);
    std::cout << "INT_MAX + 1: " << (sum ? std::to_string(*sum) : "would overflow, refused") << '\n';

    const std::string label = makeLabel(12);          // dangling view: own the characters
    std::cout << "label: " << label << '\n';

    const std::string later = std::max(std::string("rice"), std::string("noodles"));   // copy
    std::cout << "later: " << later << '\n';
    return 0;
}
