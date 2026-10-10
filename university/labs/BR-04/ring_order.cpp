// BR-04 Listing 2: the first trap, "all GPU pairs are equally connected". Two invented
// nodes: TN-4 of F6-37 (GPUs 0,1 below PCIe switch A; 2,3 below switch B) and TN-8 (two
// islands of four GPUs, fast links inside an island, one slower path between the islands).
// Model (stated, not measured): a ring all-reduce has 2(N-1) steps; in every step each GPU
// sends S/N bytes to its right neighbour at the same time; a hop inside an island uses its
// own link; all hops that cross from island g to island h share ONE path in that direction,
// so its beta is divided by the number of such hops. A step lasts as long as its slowest hop.
// All alpha and beta values are invented teaching values, not any real product.
#include <algorithm>
#include <cstdio>
#include <numeric>
#include <string>
#include <vector>

namespace {

struct Node
{
    std::string name;
    std::vector<int> island;                                     // island of each GPU
    double inAlpha, inBeta;                                      // us, GB/s inside an island
    double xAlpha, xBeta;                                        // us, GB/s of the shared crossing path
};

// Time in us of one ring step that moves chunk bytes on every hop of the ring order.
double stepUs(const Node& t, const std::vector<int>& ring, double chunk)
{
    const int n = static_cast<int>(ring.size());
    int crossings[2][2] = {{0, 0}, {0, 0}};
    for (int i = 0; i < n; ++i) {
        const int a = t.island[ring[i]];
        const int b = t.island[ring[(i + 1) % n]];
        if (a != b) {
            ++crossings[a][b];
        }
    }
    double worst = 0;
    for (int i = 0; i < n; ++i) {
        const int a = t.island[ring[i]];
        const int b = t.island[ring[(i + 1) % n]];
        const double us = a == b ? t.inAlpha + chunk / (t.inBeta * 1e3)
                                 : t.xAlpha + chunk / (t.xBeta / crossings[a][b] * 1e3);
        worst = std::max(worst, us);
    }
    return worst;
}

double ringUs(const Node& t, const std::vector<int>& ring, double bytes)
{
    const int n = static_cast<int>(ring.size());
    return 2.0 * (n - 1) * stepUs(t, ring, bytes / n);
}

std::string show(const std::vector<int>& ring)
{
    std::string s;
    for (int g : ring) {
        s += std::to_string(g) + ">";
    }
    return s + std::to_string(ring[0]);
}

void study(const Node& t, const std::vector<std::vector<int>>& orders, double bytes)
{
    const int n = static_cast<int>(t.island.size());
    std::printf("%s: %d GPUs, all-reduce of %.0f MiB; inside an island alpha %.0f us beta %.0f GB/s; "
                "crossing path alpha %.0f us beta %.0f GB/s (shared per direction)\n",
                t.name.c_str(), n, bytes / 1048576.0, t.inAlpha, t.inBeta, t.xAlpha, t.xBeta);
    const double naive = 2.0 * (n - 1) * (t.inAlpha + bytes / n / (t.inBeta * 1e3));
    std::printf("  naive prediction, every pair as fast as the best link: %10.1f us\n", naive);
    for (const auto& r : orders) {
        std::printf("  ring %-20s %10.1f us  (%.2f x naive)\n", show(r).c_str(), ringUs(t, r, bytes),
                    ringUs(t, r, bytes) / naive);
    }
    std::vector<int> p(static_cast<std::size_t>(n));
    std::iota(p.begin(), p.end(), 0);
    double best = 1e300;
    double worst = 0;
    std::vector<int> bestRing;
    std::vector<int> worstRing;
    long rings = 0;
    do {                                                         // every ring that starts at GPU 0
        const double us = ringUs(t, p, bytes);
        ++rings;
        if (us < best) {
            best = us;
            bestRing = p;
        }
        if (us > worst) {
            worst = us;
            worstRing = p;
        }
    } while (std::next_permutation(p.begin() + 1, p.end()));
    std::printf("  searched %ld ring orders: best %s %.1f us, worst %s %.1f us (worst/best %.2f)\n\n", rings,
                show(bestRing).c_str(), best, show(worstRing).c_str(), worst, worst / best);
}

}  // namespace

int main()
{
    const Node tn4{"TN-4 (F6-37)", {0, 0, 1, 1}, 6, 12, 9, 9};
    const Node tn8{"TN-8 (invented)", {0, 0, 0, 0, 1, 1, 1, 1}, 5, 40, 10, 10};
    const double big = 256.0 * 1024 * 1024;
    study(tn4, {{0, 1, 2, 3}, {0, 2, 1, 3}}, big);
    study(tn8, {{0, 1, 2, 3, 4, 5, 6, 7}, {0, 4, 1, 5, 2, 6, 3, 7}}, big);
    std::printf("small messages (64 KiB), same nodes:\n");
    study(tn8, {{0, 1, 2, 3, 4, 5, 6, 7}, {0, 4, 1, 5, 2, 6, 3, 7}}, 64.0 * 1024);
    return 0;
}
