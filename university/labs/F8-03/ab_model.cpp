// F8-03 Listing 1: the alpha-beta cost model for all-reduce, and the two bandwidths that
// benchmark reports print (algorithm bandwidth and bus bandwidth).
// Input (ab_model.in): one line "N alpha_us beta_GBps label", then message sizes in bytes.
// Every link is modelled the same: a message of m bytes takes alpha + m / beta.
#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Link
{
    double alphaUs;     // fixed cost per message, microseconds
    double betaGBps;    // bandwidth, GB/s (10^9 bytes per second)
};

double msgUs(const Link& l, double bytes)
{
    return l.alphaUs + bytes / (l.betaGBps * 1e3);   // 1 GB/s = 1e3 bytes per microsecond
}

// each formula: rounds on the critical path x (alpha + bytes per round / beta)
double ringUs(const Link& l, int n, double s) { return 2.0 * (n - 1) * msgUs(l, s / n); }
double treeUs(const Link& l, int n, double s) { return 2.0 * std::ceil(std::log2(n)) * msgUs(l, s); }
double recDoublingUs(const Link& l, int n, double s) { return std::log2(n) * msgUs(l, s); }
double halvDoublUs(const Link& l, int n, double s)
{
    double t = 0.0;
    for (double part = s / 2; part >= s / n - 1e-9; part /= 2) {   // halving: s/2, s/4, ..., s/N
        t += 2.0 * msgUs(l, part);                                 // and the same in doubling
    }
    return t;
}

std::string human(double bytes)
{
    const char* unit[] = {"B", "KiB", "MiB", "GiB"};
    int u = 0;
    while (bytes >= 1024.0 && u < 3) {
        bytes /= 1024.0;
        ++u;
    }
    char buf[32];
    std::snprintf(buf, sizeof buf, "%g %s", bytes, unit[u]);
    return buf;
}

int main()
{
    int n = 0;
    Link link{};
    std::string label;
    std::cin >> n >> link.alphaUs >> link.betaGBps >> label;
    std::vector<double> sizes;
    for (double s; std::cin >> s;) {
        sizes.push_back(s);
    }
    std::printf("model input: %s, N = %d ranks, alpha = %g us, beta = %g GB/s per link direction\n",
                label.c_str(), n, link.alphaUs, link.betaGBps);
    std::printf("%10s %11s %11s %11s %11s  %-8s %9s %9s\n", "size", "ring us", "tree us", "rec-dbl us",
                "halv-dbl us", "fastest", "algbw", "busbw");
    for (double s : sizes) {
        const double t[4] = {ringUs(link, n, s), treeUs(link, n, s), recDoublingUs(link, n, s),
                             halvDoublUs(link, n, s)};
        const char* names[4] = {"ring", "tree", "rec-dbl", "halv-dbl"};
        int best = 0;
        for (int i = 1; i < 4; ++i) {
            if (t[i] < t[best]) {
                best = i;
            }
        }
        const double algbw = s / (t[0] * 1e3);           // GB/s, for the ring
        const double busbw = algbw * 2.0 * (n - 1) / n;
        std::printf("%10s %11.2f %11.2f %11.2f %11.2f  %-8s %9.3f %9.3f\n", human(s).c_str(), t[0], t[1],
                    t[2], t[3], names[best], algbw, busbw);
    }
    // where the ring's bus bandwidth reaches half the link bandwidth: S/N = alpha * beta
    const double half = n * link.alphaUs * link.betaGBps * 1e3;
    std::printf("ring busbw reaches beta/2 at S = N * alpha * beta = %.0f bytes (%s)\n", half, human(half).c_str());
    // smallest power-of-two size from which the ring beats the tree
    for (double s = 1; s <= 1e12; s *= 2) {
        if (ringUs(link, n, s) < treeUs(link, n, s)) {
            std::printf("ring faster than the binomial tree from S = %s on\n", human(s).c_str());
            break;
        }
    }
    return 0;
}
