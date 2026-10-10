// orders_fixed.cpp: orders.cpp after the forensic lab. Ownership of the order is moved
// into the vector; a unique_ptr can be moved but not copied.
#include <cstdio>
#include <memory>
#include <string>
#include <utility>
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
    open_orders.push_back(std::move(order));  // order is now empty (nullptr)
    std::printf("%zu open order(s); order is %s\n", open_orders.size(),
                order ? "still set" : "empty after the move");
    std::printf("first: %s for table %d\n", open_orders[0]->dish.c_str(), open_orders[0]->table);
    return 0;
}
