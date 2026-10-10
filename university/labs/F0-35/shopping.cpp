#include <iostream>
#include <string>
#include <vector>

int main()
{
    std::vector<std::string> list = {"rice", "beans", "onions"};
    std::cout << "Items on the list: " << list.size() << '\n';
    std::cout << "First item: " << list[0] << '\n';

    list.push_back("lemons");
    std::cout << "After adding one, items: " << list.size() << '\n';

    for (std::size_t i = 0; i < list.size(); ++i) {
        std::cout << i << ": " << list[i] << '\n';
    }
    return 0;
}
