#include <charconv>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

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
    int bad_lines = 0;
    for (const Line& l : table5) {
        std::optional<int> n = parse_int(l.portions);
        if (!n) {
            std::cout << "cannot read portions \"" << l.portions << "\" for " << l.dish
                      << ": ask the waiter\n";
            bad_lines = bad_lines + 1;
            continue;
        }
        total = total + *n * l.price;
    }
    if (bad_lines > 0) {
        std::cout << "bill for table 5 NOT printed: " << bad_lines << " line(s) need checking\n";
        return 1;
    }
    std::cout << "total for table 5: " << total << '\n';
    return 0;
}
