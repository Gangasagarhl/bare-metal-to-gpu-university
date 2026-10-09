// hotspot.cc: makes order lines, parses them, sorts the orders by price and prints a
// total, with the time of each step. Where does the time go? Guess first, then measure.
// [[gnu::noinline]] keeps each step a separate function, so profiles can name it.
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <sstream>
#include <string>
#include <vector>

struct Order
{
    int table;
    std::string dish;
    int price_cents;
};

class Stopwatch                               // milliseconds since the last lap
{
public:
    double lap_ms()
    {
        const auto now = std::chrono::steady_clock::now();
        const std::chrono::duration<double, std::milli> d = now - last_;
        last_ = now;
        return d.count();
    }

private:
    std::chrono::steady_clock::time_point last_ = std::chrono::steady_clock::now();
};

[[gnu::noinline]] std::vector<std::string> make_lines(int count)
{
    const char* dishes[] = {"soup", "rice", "noodles", "salad", "bread"};
    std::vector<std::string> lines;
    std::uint32_t seed = 12345;
    for (int i = 0; i < count; ++i) {
        seed = seed * 1664525u + 1013904223u;            // a simple, repeatable generator
        const int table = static_cast<int>(seed % 40);
        const int price = static_cast<int>(300 + seed % 1700);
        lines.push_back(std::to_string(table) + "," + dishes[seed % 5] + "," + std::to_string(price));
    }
    return lines;
}

[[gnu::noinline]] Order parse_line(const std::string& line)
{
    std::istringstream in(line);
    std::string table, dish, price;
    std::getline(in, table, ',');
    std::getline(in, dish, ',');
    std::getline(in, price, ',');
    return {std::stoi(table), dish, std::stoi(price)};
}

[[gnu::noinline]] void sort_by_price(std::vector<Order>& orders)
{
    std::sort(orders.begin(), orders.end(),
              [](const Order& a, const Order& b) { return a.price_cents < b.price_cents; });
}

int main(int argc, char** argv)
{
    const int count = argc > 1 ? std::atoi(argv[1]) : 200000;
    Stopwatch watch;
    const std::vector<std::string> lines = make_lines(count);
    const double t_make = watch.lap_ms();
    std::vector<Order> orders;
    orders.reserve(lines.size());
    for (const std::string& line : lines) {
        orders.push_back(parse_line(line));
    }
    const double t_parse = watch.lap_ms();
    sort_by_price(orders);
    const double t_sort = watch.lap_ms();
    long long total = 0;
    for (const Order& o : orders) {
        total += o.price_cents;
    }
    std::printf("orders %zu, cheapest %d, dearest %d, total %lld cents\n", orders.size(),
                orders.front().price_cents, orders.back().price_cents, total);
    std::printf("ms: make %.1f  parse %.1f  sort %.1f\n", t_make, t_parse, t_sort);
    return 0;
}
