// orders_v2.cc - nightly order report, release 2.
#include "bench.hpp"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_set>
#include <vector>

struct Order
{
    std::string customer;  // e.g. "customer-00012345@example-shop"
    double amount;
};

struct Report
{
    double total = 0.0;
    double premiumTotal = 0.0;
    long premiumOrders = 0;
};

[[gnu::noinline]] bool isPremium(std::unordered_set<std::string> const& premium,
                                 std::string customer)
{
    return premium.count(customer) != 0;
}

[[gnu::noinline]] Report summarise(std::vector<Order> const& orders,
                                   std::unordered_set<std::string> const& premium)
{
    Report r;
    for (std::size_t k = 0; k < orders.size(); ++k) {
        r.total += orders[k].amount;
        if (isPremium(premium, orders[k].customer)) {
            r.premiumTotal += orders[k].amount;
            ++r.premiumOrders;
        }
    }
    return r;
}

std::string customerName(long id)
{
    char buf[48];
    std::snprintf(buf, sizeof buf, "customer-%08ld@example-shop", id);
    return buf;
}

int main(int argc, char** argv)
{
    int const reps = argc > 1 ? std::atoi(argv[1]) : 21;
    std::vector<Order> orders;
    for (long i = 0; i < 400'000; ++i) {
        orders.push_back({customerName((i * 7919) % 50'000), static_cast<double>(i % 97)});
    }
    std::unordered_set<std::string> premium;
    for (long id = 0; id < 50'000; id += 10) {
        premium.insert(customerName(id));
    }
    Report r;
    auto const t = bench::run([&] { r = summarise(orders, premium); bench::keep(r.total); },
                              reps > 1 ? 3 : 0, reps);
    std::printf("orders %zu, premium orders %ld, total %.0f, premium total %.0f\n",
                orders.size(), r.premiumOrders, r.total, r.premiumTotal);
    std::printf("summary step: median %.2f ms over %d runs\n", t.median * 1e3, reps);
    return 0;
}
