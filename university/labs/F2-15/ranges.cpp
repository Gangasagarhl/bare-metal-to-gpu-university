#include <algorithm>
#include <iostream>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
    int minutes;
};

int main()
{
    std::vector<Order> orders = {
        {4, "soup", 8}, {9, "curry", 25}, {2, "salad", 6}, {7, "rice", 15}};

    std::ranges::sort(orders, std::ranges::less{}, &Order::table);   // sort by the table member
    for (const Order& o : orders) {
        std::cout << o.table << ':' << o.dish << ' ';
    }
    std::cout << '\n';

    auto it = std::ranges::find(orders, "rice", &Order::dish);       // find by the dish member
    std::cout << "rice is for table " << it->table << '\n';
    return 0;
}
