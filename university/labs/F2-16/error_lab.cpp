#include <charconv>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

// Lab reference solution: one function per strategy, each with tests.
std::optional<int> parse_portions(const std::string& text)   // expected failure: optional
{
    int value = 0;
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (ec != std::errc() || end != text.data() + text.size() || value < 1 || value > 50) {
        return std::nullopt;
    }
    return value;
}

class Stock                                                    // broken invariant: exception
{
public:
    Stock(int portions, int capacity) : portions_(portions), capacity_(capacity)
    {
        if (capacity_ <= 0 || portions_ < 0 || portions_ > capacity_) {
            throw std::invalid_argument("Stock: bad starting values");
        }
    }
    int portions() const { return portions_; }

private:
    int portions_;
    int capacity_;
};

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

bool constructor_throws(int portions, int capacity)
{
    try {
        Stock s(portions, capacity);
    } catch (const std::invalid_argument&) {
        return true;
    }
    return false;
}

int main()
{
    expect(parse_portions("12") == 12, "12 is read");
    expect(!parse_portions("l2"), "letter l is refused");
    expect(!parse_portions(""), "empty text is refused");
    expect(!parse_portions("0"), "0 portions is refused");
    expect(!parse_portions("51"), "51 portions is refused");
    expect(!parse_portions("99999999999"), "too large for int is refused");
    expect(!constructor_throws(3, 10), "good stock is built");
    expect(constructor_throws(11, 10), "too many portions throws");
    expect(constructor_throws(0, 0), "no capacity throws");
    std::cout << "error handling tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
