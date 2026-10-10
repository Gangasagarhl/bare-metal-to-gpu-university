// MA102 course project - REFERENCE SOLUTION (Lab Engineer; proves the project is feasible).
// A binary <-> hex <-> decimal converter with tests. The project itself is written by learners in KID102;
// this reference uses only what MA102 and the first KID102 chapters teach: functions, loops, std::string.
// Build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined converter_ref.cpp -o converter_ref
#include <iostream>
#include <string>

// decimal -> binary text, by dividing by 2 and putting each new bit on the LEFT (F0-21)
std::string toBinary(unsigned value)
{
    if (value == 0) {
        return "0";
    }
    std::string bits;
    while (value > 0) {
        bits = std::to_string(value % 2) + bits;
        value = value / 2;
    }
    return bits;
}

// decimal -> hex text, by dividing by 16 (F0-22)
std::string toHex(unsigned value)
{
    const std::string digits = "0123456789ABCDEF";
    if (value == 0) {
        return "0";
    }
    std::string text;
    while (value > 0) {
        text = digits[value % 16] + text;
        value = value / 16;
    }
    return text;
}

// text in any base up to 16 -> decimal, by "multiply by the base and add" (F0-21 Layer 3).
// Returns false if a character is not a digit of that base.
bool fromBase(const std::string& text, unsigned base, unsigned& value)
{
    value = 0;
    if (text.empty()) {
        return false;
    }
    for (char c : text) {
        unsigned digit = 0;
        if (c >= '0' && c <= '9') {
            digit = static_cast<unsigned>(c - '0');
        } else if (c >= 'A' && c <= 'F') {
            digit = static_cast<unsigned>(c - 'A') + 10u;
        } else if (c >= 'a' && c <= 'f') {
            digit = static_cast<unsigned>(c - 'a') + 10u;
        } else {
            return false;
        }
        if (digit >= base) {
            return false;
        }
        value = value * base + digit;
    }
    return true;
}

int main()
{
    std::cout << "decimal  binary    hex\n";
    for (unsigned n : {0u, 1u, 10u, 45u, 77u, 127u, 128u, 200u, 255u, 256u, 1024u}) {
        std::cout << n << "  " << toBinary(n) << "  0x" << toHex(n) << '\n';
    }

    // Test 1: the round-trip invariant (F0-28): converting there and back gives the start number.
    unsigned roundTrips = 0;
    unsigned failures = 0;
    for (unsigned n = 0; n <= 65535u; ++n) {
        unsigned backFromBinary = 0;
        unsigned backFromHex = 0;
        const bool okB = fromBase(toBinary(n), 2, backFromBinary);
        const bool okH = fromBase(toHex(n), 16, backFromHex);
        ++roundTrips;
        if (!okB || !okH || backFromBinary != n || backFromHex != n) {
            ++failures;
        }
    }
    std::cout << "round trips tested: " << roundTrips << ", failures: " << failures << '\n';

    // Test 2: known values from the MA102 chapters (F0-21 and F0-22 worked examples).
    unsigned v = 0;
    int known = 0;
    int knownFailures = 0;
    ++known; if (!(fromBase("101101", 2, v) && v == 45)) ++knownFailures;
    ++known; if (!(fromBase("11001000", 2, v) && v == 200)) ++knownFailures;
    ++known; if (!(fromBase("C8", 16, v) && v == 200)) ++knownFailures;
    ++known; if (!(fromBase("2A", 16, v) && v == 42)) ++knownFailures;
    ++known; if (!(toHex(255) == "FF")) ++knownFailures;
    ++known; if (!(toBinary(12) == "1100")) ++knownFailures;
    std::cout << "known values tested: " << known << ", failures: " << knownFailures << '\n';

    // Test 3: bad input is refused, not guessed.
    int refused = 0;
    if (!fromBase("102", 2, v)) ++refused;    // 2 is not a binary digit
    if (!fromBase("G1", 16, v)) ++refused;    // G is not a hex digit
    if (!fromBase("", 10, v)) ++refused;      // empty text
    std::cout << "bad inputs refused: " << refused << " of 3\n";
    return 0;
}
