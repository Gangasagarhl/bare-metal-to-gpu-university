#include <iostream>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
};

// Forensic evidence: the order screen keeps a reference to the first order.
int main()
{
    std::cout << std::unitbuf;   // print each line at once, so the log survives a crash
    std::vector<Order> orders = {{2, "rice"}, {5, "soup"}};
    const Order& first = orders[0];
    std::cout << "first order: table " << first.table << '\n';
    std::cout << "size " << orders.size() << ", capacity " << orders.capacity() << '\n';

    orders.push_back({7, "tea"});   // a new order arrives
    std::cout << "size " << orders.size() << ", capacity " << orders.capacity() << '\n';

    std::cout << "first order again: table " << first.table << '\n';
    return 0;
}
