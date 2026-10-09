#include <iostream>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
};

int main()
{
    std::vector<Order> orders = {{2, "rice"}, {5, "soup"}};
    const std::size_t first = 0;   // remember the position, not a reference to the element
    std::cout << "first order: table " << orders[first].table << '\n';

    orders.push_back({7, "tea"});
    std::cout << "first order again: table " << orders[first].table << '\n';
    return 0;
}
