#include <iostream>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
};

// Returns a pointer to the first order for a table, or nullptr if there is none.
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
    std::cout << std::unitbuf;   // print each line at once, so the log survives a crash
    const std::vector<Order> orders = {{2, "rice"}, {5, "soup"}};
    for (const int table : {5, 2, 9}) {
        const Order* found = findOrder(orders, table);
        std::cout << "table " << table << " ordered " << found->dish << '\n';   // no check!
    }
    return 0;
}
