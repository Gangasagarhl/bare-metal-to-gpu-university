// Pointer chasing: the average time of ONE load whose address depends on the load before it,
// for working sets of growing size. Each node fills one 64-byte block so every load touches
// a new block. Mode "random" links the nodes in a shuffled cycle; mode "sequential" links
// node i to node i+1 (a pattern the hardware can predict). Numbers are measurements, not specs.
#include <chrono>
#include <cstddef>
#include <cstdio>
#include <random>
#include <string>
#include <utility>
#include <vector>

struct alignas(64) Node
{
    std::size_t next;
    char pad[64 - sizeof(std::size_t)];
};

// Choose a visiting order (shuffled, or 0, 1, 2, ... for "sequential"), then link the nodes
// in that order; the last links back to the first, so the links form ONE cycle through all n.
std::vector<Node> makeChain(std::size_t n, bool random)
{
    std::vector<std::size_t> order(n);
    for (std::size_t i = 0; i < n; ++i) {
        order[i] = i;
    }
    if (random) {
        std::mt19937_64 rng(12345);
        for (std::size_t i = n - 1; i > 0; --i) {
            std::uniform_int_distribution<std::size_t> pick(0, i - 1);
            std::swap(order[i], order[pick(rng)]);
        }
    }
    std::vector<Node> nodes(n);
    for (std::size_t i = 0; i < n; ++i) {
        nodes[order[i]].next = order[(i + 1) % n];
    }
    return nodes;
}

int main(int argc, char** argv)
{
    const bool random = !(argc > 1 && std::string(argv[1]) == "sequential");
    const std::size_t loads = 4'000'000;
    std::printf("mode: %s chain, %zu dependent loads per size\n",
                random ? "random" : "sequential", loads);
    std::printf("%14s %12s\n", "working set", "ns per load");
    std::size_t sink = 0;
    for (std::size_t bytes = 4 * 1024; bytes <= 256u * 1024 * 1024; bytes *= 2) {
        const std::vector<Node> nodes = makeChain(bytes / sizeof(Node), random);
        std::size_t p = 0;
        for (std::size_t i = 0; i < nodes.size(); ++i) {  // warm-up: touch every node once
            p = nodes[p].next;
        }
        const auto t0 = std::chrono::steady_clock::now();
        for (std::size_t i = 0; i < loads; ++i) {
            p = nodes[p].next;  // the next address is known only after this load returns
        }
        const auto t1 = std::chrono::steady_clock::now();
        sink += p;
        const double ns = std::chrono::duration<double, std::nano>(t1 - t0).count() / loads;
        std::printf("%10zu KiB %12.2f\n", bytes / 1024, ns);
    }
    std::printf("(checksum %zu)\n", sink);
    return 0;
}
