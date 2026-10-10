#include <charconv>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

// Forensic evidence: the bill printer. Each line of an order is "<portions> <dish> <price>".
std::optional<int> parse_int(const std::string& text)
{
    int value = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

struct Line
{
    std::string portions;
    std::string dish;
    int price;
};

int main()
{
    std::vector<Line> table5 = {{"2", "soup", 6}, {"l2", "dumplings", 1}, {"1", "tea", 2}};
    int total = 0;
    for (const Line& l : table5) {
        int n = parse_int(l.portions).value_or(0);   // "never crash on bad input"
        std::cout << n << " x " << l.dish << " at " << l.price << " = " << n * l.price << '\n';
        total = total + n * l.price;
    }
    std::cout << "total for table 5: " << total << '\n';
    return 0;
}
