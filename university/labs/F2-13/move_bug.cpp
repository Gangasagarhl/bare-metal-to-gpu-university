#include <iostream>
#include <string>
#include <utility>
#include <vector>

// Forensic evidence: the order desk after a "performance clean-up".
struct Order
{
    int table;
    std::string dish;
};

std::vector<Order> kitchen_queue;

void print_receipt(const Order& o)
{
    std::cout << "receipt: table " << o.table << ", dish '" << o.dish << "'\n";
}

void take_order(int table, std::string dish)
{
    Order o{table, dish};
    kitchen_queue.push_back(std::move(o));   // the clean-up: move instead of copy
    print_receipt(o);
}

int main()
{
    take_order(3, "vegetable curry");
    take_order(8, "lentil soup with bread");
    std::cout << "kitchen queue:\n";
    for (const Order& o : kitchen_queue) {
        std::cout << "  table " << o.table << ": " << o.dish << '\n';
    }
    return 0;
}
