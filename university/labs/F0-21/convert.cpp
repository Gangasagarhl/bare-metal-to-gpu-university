// Decimal to binary by dividing by 2, and binary to decimal by doubling.
#include <iostream>
#include <string>

std::string toBinary(int number)
{
    if (number == 0) {
        return "0";
    }
    std::string bits;
    while (number > 0) {
        int remainder = number % 2;
        std::cout << "  " << number << " / 2 = " << number / 2
                  << " remainder " << remainder << '\n';
        bits = std::to_string(remainder) + bits;  // new bit goes on the LEFT
        number = number / 2;
    }
    return bits;
}

int toDecimal(const std::string& bits)
{
    int value = 0;
    for (char bit : bits) {
        value = value * 2 + (bit - '0');  // double what you have, add the new bit
        std::cout << "  read " << bit << ", value is now " << value << '\n';
    }
    return value;
}

int main()
{
    std::cout << "45 to binary:\n";
    const std::string a = toBinary(45);
    std::cout << "45 = " << a << " in binary\n\n";

    std::cout << "200 to binary:\n";
    const std::string b = toBinary(200);
    std::cout << "200 = " << b << " in binary\n\n";

    std::cout << "10110 to decimal:\n";
    const int c = toDecimal("10110");
    std::cout << "10110 = " << c << " in decimal\n";
    return 0;
}
