#include <charconv>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

// Way 1: an error code. The caller must look at the returned value.
enum class ParseError { ok, empty, not_a_number, out_of_range };

ParseError parse_code(const std::string& text, int& out)
{
    if (text.empty()) {
        return ParseError::empty;
    }
    auto [end, ec] = std::from_chars(text.data(), text.data() + text.size(), out);
    if (ec == std::errc::result_out_of_range) {
        return ParseError::out_of_range;
    }
    if (ec != std::errc() || end != text.data() + text.size()) {
        return ParseError::not_a_number;
    }
    return ParseError::ok;
}

// Way 2: an exception. std::stoi throws when the text does not start with a number.
int parse_throw(const std::string& text)
{
    std::size_t used = 0;
    int value = std::stoi(text, &used);
    if (used != text.size()) {
        throw std::invalid_argument("extra characters after the number: " + text);
    }
    return value;
}

// Way 3: std::optional. Either an int or nothing.
std::optional<int> parse_optional(const std::string& text)
{
    int value = 0;
    if (parse_code(text, value) != ParseError::ok) {
        return std::nullopt;
    }
    return value;
}

int main()
{
    const std::string inputs[] = {"12", "l2", "", "99999999999"};
    for (const std::string& text : inputs) {
        std::cout << "input \"" << text << "\"\n";

        int value = 0;
        ParseError e = parse_code(text, value);
        std::cout << "  error code: " << static_cast<int>(e);
        if (e == ParseError::ok) {
            std::cout << " value " << value;
        }
        std::cout << '\n';

        try {
            int v = parse_throw(text);
            std::cout << "  exception:  value " << v << '\n';
        } catch (const std::invalid_argument& ex) {
            std::cout << "  exception:  caught std::invalid_argument, what() = " << ex.what()
                      << '\n';
        } catch (const std::out_of_range& ex) {
            std::cout << "  exception:  caught std::out_of_range, what() = " << ex.what() << '\n';
        }

        std::optional<int> o = parse_optional(text);
        if (o) {
            std::cout << "  optional:   value " << *o << '\n';
        } else {
            std::cout << "  optional:   empty\n";
        }
    }
    return 0;
}
