// route.cpp - the decision every IPv4 host makes for every packet it sends:
// find the most specific matching route (longest prefix), then decide whose MAC address
// to ask for with ARP: the destination itself (on-link) or the next-hop router.
#include <cstdint>
#include <iostream>
#include <string>
#include <vector>

struct Route
{
    std::uint32_t network;
    int prefix;
    std::uint32_t gateway;   // 0 means "on-link": the destination is on this wire
    std::string note;
};

constexpr std::uint32_t ip(unsigned a, unsigned b, unsigned c, unsigned d)
{
    return (a << 24) | (b << 16) | (c << 8) | d;
}

std::string dotted(std::uint32_t a)
{
    return std::to_string(a >> 24) + '.' + std::to_string((a >> 16) & 0xFF) + '.' +
           std::to_string((a >> 8) & 0xFF) + '.' + std::to_string(a & 0xFF);
}

std::uint32_t maskOf(int prefix)
{
    return prefix == 0 ? 0u : ~std::uint32_t{0} << (32 - prefix);
}

const Route* lookup(const std::vector<Route>& table, std::uint32_t dst)
{
    const Route* best = nullptr;
    for (const Route& r : table) {
        const bool matches = (dst & maskOf(r.prefix)) == r.network;
        if (matches && (best == nullptr || r.prefix > best->prefix)) {
            best = &r;                          // longer prefix = more specific = wins
        }
    }
    return best;
}

int main()
{
    // The table of Zainab's laptop, address 192.168.41.15/24 in the office.
    const std::vector<Route> table = {
        {ip(0, 0, 0, 0), 0, ip(192, 168, 41, 1), "default route"},
        {ip(192, 168, 41, 0), 24, 0, "own subnet"},
        {ip(10, 8, 0, 0), 16, ip(192, 168, 41, 254), "lab network behind a second router"},
        {ip(10, 8, 3, 0), 24, ip(192, 168, 41, 253), "one lab room with its own router"},
    };
    const std::vector<std::uint32_t> destinations = {
        ip(192, 168, 41, 77), ip(192, 168, 40, 20), ip(10, 8, 7, 7), ip(10, 8, 3, 9),
        ip(203, 0, 113, 9)};

    for (std::uint32_t dst : destinations) {
        const Route* r = lookup(table, dst);
        const std::uint32_t nextHop = r->gateway == 0 ? dst : r->gateway;
        std::cout << "to " << dotted(dst) << ": route " << dotted(r->network) << '/' << r->prefix
                  << " (" << r->note << "), ARP for " << dotted(nextHop)
                  << (r->gateway == 0 ? " (on-link)" : " (router)") << '\n';
    }
    return 0;
}
