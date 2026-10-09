#include <iostream>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
};

const Order* findOrder(const std::vector<Order>& orders, int table)
{
    for (const Order& o : orders) {
        if (o.table == table) {
            return &o;
        }
    }
    return nullptr;
}

int main()
{
    const std::vector<Order> orders = {{2, "rice"}, {5, "soup"}};
    for (const int table : {5, 2, 9}) {
        const Order* found = findOrder(orders, table);
        if (found == nullptr) {
            std::cout << "table " << table << " has no order\n";
            continue;
        }
        std::cout << "table " << table << " ordered " << found->dish << '\n';
    }
    return 0;
}
