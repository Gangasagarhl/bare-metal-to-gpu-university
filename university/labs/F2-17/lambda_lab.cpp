#include <algorithm>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

// Lab reference solution: lambdas with the standard algorithms, and a lambda factory.
struct Order
{
    int table;
    std::string dish;
    int minutes;
};

// Returns a test "is this order slower than limit?" The limit is captured by value on purpose:
// the returned lambda outlives this function, so a reference to limit would dangle.
std::function<bool(const Order&)> slower_than(int limit)
{
    return [limit](const Order& o) { return o.minutes > limit; };
}

int failures = 0;

void expect(bool condition, const char* what)
{
    if (!condition) {
        std::cout << "FAIL: " << what << '\n';
        failures = failures + 1;
    }
}

int main()
{
    std::vector<Order> orders = {
        {4, "soup", 8}, {9, "curry", 25}, {2, "salad", 6}, {7, "rice", 15}, {3, "tea", 6}};

    // Sort by minutes, and by table number when the minutes are equal.
    std::sort(orders.begin(), orders.end(), [](const Order& a, const Order& b) {
        if (a.minutes != b.minutes) {
            return a.minutes < b.minutes;
        }
        return a.table < b.table;
    });
    expect(orders[0].table == 2 && orders[1].table == 3, "ties broken by table number");
    expect(orders.back().dish == "curry", "slowest last");

    auto slow = slower_than(10);
    expect(std::count_if(orders.begin(), orders.end(), slow) == 2, "two orders over 10 minutes");

    int served = 0;
    std::for_each(orders.begin(), orders.end(), [&served](const Order&) { served += 1; });
    expect(served == 5, "by-reference capture counts every order");

    auto it = std::find_if(orders.begin(), orders.end(),
                           [](const Order& o) { return o.dish == "rice"; });
    expect(it != orders.end() && it->table == 7, "find_if finds the rice order");
    std::cout << "lambda tests: " << failures << " failures\n";
    return failures == 0 ? 0 : 1;
}
