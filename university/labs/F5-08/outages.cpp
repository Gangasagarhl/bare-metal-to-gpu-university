// outages.cpp - generates the evidence pack of the F5-08 forensic lab "two copies, still down".
// A simulated year of six machines in two racks. Every machine has its own independent
// outages, and (the planted fault, described only in the answer key) the power feed of
// one rack sometimes fails, taking every machine in that rack down at the same time.
// The "orders" service keeps two full copies, on m1 and m2; it is down only when both are.
#include <array>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

struct Rng
{
    std::uint64_t state;
    std::uint64_t next()
    {
        std::uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    double uniform() { return static_cast<double>(next() >> 11) * 0x1.0p-53; }
    int below(int n) { return static_cast<int>(uniform() * n); }
};

constexpr int kHours = 365 * 24;
constexpr int kMachines = 6;

struct Outage
{
    int start;      // hour of the year
    int length;     // hours
    int machine;    // 0..5, or -1 for a whole-rack event (expanded below)
};

std::string when(int hour)
{
    char buf[32];
    std::snprintf(buf, sizeof buf, "day %03d %02d:00", hour / 24 + 1, hour % 24);
    return buf;
}

int main()
{
    Rng rng{41};
    std::vector<std::array<bool, kHours>> down(kMachines);
    for (auto& d : down) {
        d.fill(false);
    }
    std::vector<Outage> log;
    // independent outages: on each day, each machine starts an outage with probability 1/150
    for (int day = 0; day < 365; ++day) {
        for (int m = 0; m < kMachines; ++m) {
            if (rng.uniform() < 1.0 / 150.0) {
                log.push_back({day * 24 + rng.below(24), 2 + rng.below(5), m});
            }
        }
    }
    // planted fault: rack A's power feed (m1, m2, m3) fails on three days
    for (int i = 0; i < 3; ++i) {
        const int start = (40 + 110 * i + rng.below(30)) * 24 + rng.below(24);
        const int length = 1 + rng.below(3);
        for (int m = 0; m < 3; ++m) {
            log.push_back({start, length, m});
        }
    }
    for (const Outage& o : log) {
        for (int h = o.start; h < o.start + o.length && h < kHours; ++h) {
            down[o.machine][h] = true;
        }
    }
    // print machine outages in time order (simple insertion order by start, then machine)
    std::vector<Outage> sorted = log;
    for (std::size_t i = 1; i < sorted.size(); ++i) {
        for (std::size_t j = i; j > 0; --j) {
            const Outage& x = sorted[j - 1];
            const Outage& y = sorted[j];
            if (x.start > y.start || (x.start == y.start && x.machine > y.machine)) {
                std::swap(sorted[j - 1], sorted[j]);
            }
        }
    }
    std::printf("=== machines.log: machine outages (rack A = m1 m2 m3, rack B = m4 m5 m6) ===\n");
    for (const Outage& o : sorted) {
        std::printf("%s  m%d  down for %d h\n", when(o.start).c_str(), o.machine + 1, o.length);
    }
    std::printf("\n=== orders.log: service 'orders' (copies on m1 and m2) unavailable ===\n");
    int serviceDown = 0;
    for (int h = 0; h < kHours; ++h) {
        if (down[0][h] && down[1][h]) {
            ++serviceDown;
            if (h == 0 || !(down[0][h - 1] && down[1][h - 1])) {
                std::printf("%s  orders DOWN (m1 and m2 both down)\n", when(h).c_str());
            }
        }
    }
    std::printf("\n=== summary ===\n");
    for (int m = 0; m < kMachines; ++m) {
        int hours = 0;
        for (int h = 0; h < kHours; ++h) {
            hours += down[m][h] ? 1 : 0;
        }
        std::printf("m%d  down %3d of %d hours  availability %.5f\n", m + 1, hours, kHours,
                    1.0 - 1.0 * hours / kHours);
    }
    std::printf("orders  down %3d of %d hours  availability %.5f\n", serviceDown, kHours,
                1.0 - 1.0 * serviceDown / kHours);
    return 0;
}
