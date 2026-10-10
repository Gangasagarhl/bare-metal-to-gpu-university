// BR-03 Listing 3: the bug that carries over unchanged. Four threads, one plain table,
// ++bins[b] from every thread: a data race (F2-35). Built by run.sh twice: plain -O2
// (count what was lost) and with ThreadSanitizer (let the tool name the race).
#include <array>
#include <cstdio>
#include <thread>
#include <vector>

int main()
{
    const std::size_t n = std::size_t{1} << 22;
    std::vector<unsigned char> in(n);
    for (std::size_t i = 0; i < n; ++i) {
        in[i] = static_cast<unsigned char>((static_cast<unsigned>(i) * 2654435761u) >> 24);
    }
    std::array<unsigned long, 256> bins{};  // plain counters, shared by every thread: the bug
    const unsigned T = 4;
    std::vector<std::thread> cooks;
    for (unsigned t = 0; t < T; ++t) {
        cooks.emplace_back([&in, &bins, t, T, n] {
            for (std::size_t i = n * t / T; i < n * (t + 1) / T; ++i) {
                ++bins[in[i]];  // load, add 1, store: three steps another thread can cut into
            }
        });
    }
    for (std::thread& c : cooks) {
        c.join();
    }
    unsigned long total = 0;
    for (unsigned long b : bins) {
        total += b;
    }
    std::printf("counted %lu of %zu bytes; %ld increments lost\n", total, n,
                static_cast<long>(n) - static_cast<long>(total));
    return 0;
}
