#include <cstddef>
#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> menu{"soup", "noodles"};
    menu.push_back("rice");              // the vector grows by one element
    menu.push_back("salad");

    std::cout << "The menu has " << menu.size() << " dishes.\n";
    std::cout << "First: " << menu.front() << ", last: " << menu.back() << '\n';

    // Index loop: i runs from 0 to size() - 1.
    for (std::size_t i = 0; i < menu.size(); ++i) {
        std::cout << "  " << i << ": " << menu[i] << '\n';
    }

    menu.pop_back();                     // remove the last dish
    menu.at(1) = "fried noodles";        // at() checks the index before using it

    // Range-for: no index needed. const& reads each string without copying it.
    for (const std::string& dish : menu) {
        std::cout << "  * " << dish << '\n';
    }
    std::cout << "Empty? " << (menu.empty() ? "yes" : "no") << '\n';
    return 0;
}
