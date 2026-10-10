#include <cstddef>
#include <iostream>
#include <string>

int main()
{
    std::string order = "table 7: 2 soups";
    std::cout << "order = \"" << order << "\", size " << order.size() << '\n';

    std::size_t colon = order.find(':');           // position of the first ':'
    std::string table = order.substr(0, colon);     // characters before it
    std::string rest = order.substr(colon + 2);     // skip ": "
    std::cout << "table part: \"" << table << "\"\n";
    std::cout << "rest part:  \"" << rest << "\"\n";

    int count = std::stoi(rest);                    // text "2 soups" -> number 2
    std::cout << "count + 1 = " << count + 1 << '\n';

    std::string reply = "OK, " + std::to_string(count) + " coming";
    reply += "!";
    std::cout << reply << '\n';

    if (order.find("tea") == std::string::npos) {
        std::cout << "No tea in this order.\n";
    }
    std::cout << std::boolalpha << "\"apple\" < \"banana\" is "
              << (std::string("apple") < std::string("banana")) << '\n';
    return 0;
}
