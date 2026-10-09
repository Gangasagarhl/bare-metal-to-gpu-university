#include <algorithm>
#include <iostream>
#include <numeric>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
    int minutes;
};

bool takes_longer(const Order& a, const Order& b)   // used by sort: true if a goes first
{
    return a.minutes > b.minutes;
}

bool is_quick(const Order& o)                       // used by count_if
{
    return o.minutes <= 10;
}

int main()
{
    std::vector<int> minutes = {12, 5, 20, 8, 5};

    std::sort(minutes.begin(), minutes.end());
    std::cout << "sorted:";
    for (int m : minutes) {
        std::cout << ' ' << m;
    }
    std::cout << '\n';

    int total = std::accumulate(minutes.begin(), minutes.end(), 0);
    auto longest = std::max_element(minutes.begin(), minutes.end());
    auto fives = std::count(minutes.begin(), minutes.end(), 5);
    auto found = std::find(minutes.begin(), minutes.end(), 20);
    auto missing = std::find(minutes.begin(), minutes.end(), 99);
    std::cout << "total " << total << ", longest " << *longest << ", fives " << fives << '\n';
    std::cout << "20 found at position " << (found - minutes.begin())
              << "; 99 found: " << (missing != minutes.end()) << '\n';

    std::vector<Order> orders = {
        {4, "soup", 8}, {9, "curry", 25}, {2, "salad", 6}, {7, "rice", 15}};
    std::sort(orders.begin(), orders.end(), takes_longer);
    std::cout << "start first:";
    for (const Order& o : orders) {
        std::cout << ' ' << o.dish << '(' << o.minutes << ')';
    }
    std::cout << '\n';
    std::cout << "quick orders: " << std::count_if(orders.begin(), orders.end(), is_quick) << '\n';
    return 0;
}
