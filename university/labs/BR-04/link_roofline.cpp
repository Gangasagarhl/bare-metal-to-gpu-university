// BR-04 Listing 1: roofline thinking applied to links. For each link (alpha in us, beta in
// GB/s, read from standard input) it prints, for message sizes from 8 B to 1 GiB:
//   the roof     min(beta, n / alpha)      - the two straight lines of a roofline;
//   the model    n / (alpha + n / beta)    - what the alpha-beta model says you achieve;
// the ridge (half-bandwidth size) n = alpha * beta, and which link is fastest at each size.
// Input lines: <name> <alpha_us> <beta_GBps>. The values in link_roofline.in are invented
// teaching values (TN-4 of F6-37 plus two made-up links X and Y), not any real product.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

namespace {

struct Link
{
    std::string name;
    double alphaUs = 0;
    double betaGBps = 0;
};

// Achieved bandwidth in GB/s for one message of n bytes (1 GB/s = 1000 bytes per us).
double achieved(const Link& l, double n)
{
    return n / (l.alphaUs + n / (l.betaGBps * 1e3)) / 1e3;
}

double roof(const Link& l, double n)
{
    const double latencyLine = n / l.alphaUs / 1e3;            // bytes per alpha, in GB/s
    return latencyLine < l.betaGBps ? latencyLine : l.betaGBps;
}

std::string sizeName(double n)
{
    const char* unit[] = {"B", "KiB", "MiB", "GiB"};
    int u = 0;
    while (n >= 1024 && u < 3) {
        n /= 1024;
        ++u;
    }
    return std::to_string(static_cast<long>(n)) + " " + unit[u];
}

}  // namespace

int main()
{
    std::vector<Link> links;
    Link l;
    while (std::cin >> l.name >> l.alphaUs >> l.betaGBps) {
        links.push_back(l);
    }
    if (links.empty()) {
        std::printf("no links on standard input\n");
        return 1;
    }
    for (std::size_t i = 0; i < links.size(); ++i) {
        std::printf("link %zu: %-28s alpha = %6.2f us  beta = %7.3f GB/s  ridge alpha*beta = %s\n", i,
                    links[i].name.c_str(), links[i].alphaUs, links[i].betaGBps,
                    sizeName(links[i].alphaUs * links[i].betaGBps * 1e3).c_str());
    }
    std::printf("\nachieved GB/s by the model (roof in brackets); last column: fastest link at that size\n");
    std::printf("%9s", "size");
    for (std::size_t i = 0; i < links.size(); ++i) {
        std::printf("   link %zu (roof)   ", i);
    }
    std::printf(" fastest\n");
    for (double n = 8; n <= 1024.0 * 1024 * 1024; n *= 8) {
        std::printf("%9s", sizeName(n).c_str());
        std::size_t best = 0;
        for (std::size_t i = 0; i < links.size(); ++i) {
            std::printf("  %8.3f (%7.3f) ", achieved(links[i], n), roof(links[i], n));
            if (achieved(links[i], n) > achieved(links[best], n)) {
                best = i;
            }
        }
        std::printf(" link %zu\n", best);
    }
    return 0;
}
