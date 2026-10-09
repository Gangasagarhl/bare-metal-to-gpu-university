#include <charconv>
#include <expected>
#include <iostream>
#include <string>

// C++23 only: a value or an error, in one return type. Built by run.sh with -std=c++23.
std::expected<int, std::string> parse_portions(const std::string& text)
{
    int value = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size()) {
        return std::unexpected("not a number: \"" + text + "\"");
    }
    return value;
}

int main()
{
    for (const std::string text : {"12", "l2"}) {
        auto r = parse_portions(text);
        if (r) {
            std::cout << "value " << *r << '\n';
        } else {
            std::cout << "error " << r.error() << '\n';
        }
    }
    return 0;
}
