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

    // 1. No capture: the comparison of F2-15, written where it is used.
    std::sort(orders.begin(), orders.end(),
              [](const Order& a, const Order& b) { return a.minutes > b.minutes; });
    std::cout << "longest first: " << orders.front().dish << '\n';

    // 2. Capture by value: the lambda keeps its own copy of limit.
    int limit = 10;
    auto quick = [limit](const Order& o) { return o.minutes <= limit; };
    std::cout << "quick orders: " << std::count_if(orders.begin(), orders.end(), quick) << '\n';

    // 3. Capture by reference: the lambda changes total in main.
    int total = 0;
    std::for_each(orders.begin(), orders.end(), [&total](const Order& o) { total += o.minutes; });
    std::cout << "total minutes: " << total << '\n';

    // 4. A lambda stored in a variable and called like a function.
    auto label = [](const Order& o) { return "table " + std::to_string(o.table) + ": " + o.dish; };
    std::cout << label(orders.back()) << '\n';

    // 5. A generic lambda: auto parameters make it work for many types.
    auto twice = [](auto x) { return x + x; };
    std::cout << twice(21) << ' ' << twice(std::string("ha")) << '\n';

    // 6. mutable: the lambda's own copy of next may change between calls.
    auto ticket_number = [next = 100]() mutable { return next++; };
    int t1 = ticket_number();
    int t2 = ticket_number();
    int t3 = ticket_number();
    std::cout << "tickets: " << t1 << ' ' << t2 << ' ' << t3 << '\n';
    return 0;
}
