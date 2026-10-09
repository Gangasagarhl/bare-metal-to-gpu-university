// orders.cpp: keeps the open orders of a kitchen. It does not compile.
#include <cstdio>
#include <memory>
#include <string>
#include <vector>

struct Order
{
    std::string dish;
    int table;
};

int main()
{
    std::vector<std::unique_ptr<Order>> open_orders;
    auto order = std::make_unique<Order>(Order{"noodle soup", 4});
    open_orders.push_back(order);
    std::printf("%zu open order(s)\n", open_orders.size());
    return 0;
}
