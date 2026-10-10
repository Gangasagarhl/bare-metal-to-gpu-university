// F8-10 Listing 2: alpha-beta model of a flat ring all-reduce over all P = N x G GPUs
// versus the hierarchical all-reduce of Listing 1 (intra-node reduce-scatter, inter-node
// ring all-reduce of each shard, intra-node all-gather).
// Assumptions (state them whenever you quote a number from this model):
//   - every message of m bytes on a link costs alpha + m / beta;
//   - each node has ONE network interface; inter-node flows that leave a node at the same
//     time share its bandwidth equally;
//   - the flat ring is ordered node by node, so it crosses the network once per node;
//     a ring step lasts as long as its slowest link, which is a network link;
//   - intra-node links are all equal and do not share bandwidth.
// The column "overlap us" is max(intra, inter): the bound for perfectly pipelined phases.
// Input: label, four "name value" lines, then lines "N G" (nodes, GPUs per node).
#include <cstdio>
#include <iostream>
#include <map>
#include <string>

namespace {

double linkUs(double alphaUs, double gbps, double bytes)
{
    return alphaUs + bytes / (gbps * 1e3);   // 1 GB/s = 1,000 bytes per microsecond
}

}  // namespace

int main()
{
    std::string label;
    std::cin >> label;
    std::map<std::string, double> p;
    for (int i = 0; i < 4; ++i) {
        std::string name;
        double v = 0.0;
        std::cin >> name >> v;
        p[name] = v;
    }
    const double ai = p["intra_alpha_us"];
    const double bi = p["intra_GBps"];
    const double ae = p["inter_alpha_us"];
    const double be = p["inter_GBps"];
    std::printf("model input: %s\n", label.c_str());
    std::printf("intra-node: alpha %.1f us, %.1f GB/s; inter-node: alpha %.1f us, %.1f GB/s per node\n",
                ai, bi, ae, be);
    const double sizes[] = {8.0 * 1024, 1024.0 * 1024, 64.0 * 1024 * 1024, 1024.0 * 1024 * 1024};
    const char* names[] = {"8 KiB", "1 MiB", "64 MiB", "1 GiB"};
    int n = 0;
    int g = 0;
    while (std::cin >> n >> g) {
        const int ranks = n * g;
        std::printf("N = %d nodes x G = %d GPUs (P = %d)\n", n, g, ranks);
        std::printf("  %7s %11s %11s %11s %11s %11s %9s\n", "size", "flat us", "hier us",
                    "intra us", "inter us", "overlap us", "flat/hier");
        for (int k = 0; k < 4; ++k) {
            const double s = sizes[k];
            // Flat ring: 2(P-1) steps of S/P bytes; the slowest link of each step is the network.
            const double flat = 2.0 * (ranks - 1) * linkUs(ae, be, s / ranks);
            // Hierarchical: reduce-scatter and all-gather inside the node, 2(G-1) steps of S/G;
            // between nodes, G concurrent rings of 2(N-1) steps of S/(G N) bytes sharing the NIC,
            // which costs the same time as one ring of S/N-byte steps at full bandwidth.
            const double intra = (g > 1) ? 2.0 * (g - 1) * linkUs(ai, bi, s / g) : 0.0;
            const double inter = (n > 1) ? 2.0 * (n - 1) * linkUs(ae, be, s / n) : 0.0;
            const double hier = intra + inter;
            // If the phases are pipelined over chunks, the step cannot be faster than its
            // slower phase: a lower bound, not a prediction.
            const double overlap = intra > inter ? intra : inter;
            std::printf("  %7s %11.1f %11.1f %11.1f %11.1f %11.1f %9.2f\n", names[k], flat, hier,
                        intra, inter, overlap, flat / hier);
        }
        // Bytes each node's network interface sends per all-reduce, as a multiple of S.
        std::printf("  bytes sent per NIC / S: flat %.3f, hierarchical %.3f\n",
                    2.0 * (ranks - 1) / ranks, 2.0 * (n - 1) / n);
    }
    return 0;
}
