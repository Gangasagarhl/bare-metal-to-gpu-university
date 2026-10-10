// F6-37 Listing 3: the expected peer matrix of TN-4, the university's invented teaching node:
// four TG-1 GPUs. GPUs 0-1 sit behind one PCIe switch, GPUs 2-3 behind another, and the two
// switches meet at the CPU's root complex. All link numbers are INVENTED for arithmetic.
// A copy over a path costs alpha + bytes / beta (the alpha-beta model of F1-62). If peer
// access is not enabled for a pair, the copy is staged through host memory: two host-link
// copies one after the other.
#include <cstdio>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <utility>

struct Link
{
    const char* type;
    double alphaUs;     // fixed cost per copy, microseconds (invented)
    double betaGBs;     // bandwidth, GB/s = 1e9 bytes per second (invented)
};

const Link kSwitch = {"PCIe switch", 6.0, 12.0};
const Link kRootComplex = {"root complex", 9.0, 9.0};
const Link kHostLink = {"host link (TG-1)", 10.0, 16.0};

const Link& pathOf(int a, int b)
{
    return (a / 2 == b / 2) ? kSwitch : kRootComplex;    // same switch: GPUs {0,1} or {2,3}
}

double copyUs(const Link& l, double bytes)
{
    return l.alphaUs + bytes / (l.betaGBs * 1e3);       // GB/s = 1e3 bytes per microsecond
}

int main()
{
    std::string name;
    double bytes = 0.0;
    std::string disabledList;
    while (std::cin >> name >> bytes >> disabledList) {
        std::set<std::pair<int, int>> disabled;          // "src>dst" pairs whose peer access is off
        if (disabledList != "-") {
            std::stringstream ss(disabledList);
            std::string item;
            while (std::getline(ss, item, ',')) {
                disabled.insert({item[0] - '0', item[2] - '0'});
            }
        }
        std::printf("%s: %.0f bytes, peer access off for: %s\n", name.c_str(), bytes, disabledList.c_str());
        std::printf("  %-8s", "src\\dst");
        for (int b = 0; b < 4; ++b) {
            std::printf("%10d", b);
        }
        std::printf("    (%s)\n", bytes < 1e6 ? "time in us" : "GB/s");
        for (int a = 0; a < 4; ++a) {
            std::printf("  %-8d", a);
            for (int b = 0; b < 4; ++b) {
                if (a == b) {
                    std::printf("%10s", "-");
                    continue;
                }
                const bool staged = disabled.count({a, b}) > 0;
                const double us = staged ? 2.0 * copyUs(kHostLink, bytes) : copyUs(pathOf(a, b), bytes);
                if (bytes < 1e6) {
                    std::printf("%9.1f%s", us, staged ? "*" : " ");
                } else {
                    std::printf("%9.2f%s", bytes / (us * 1e3), staged ? "*" : " ");
                }
            }
            std::printf("\n");
        }
    }
    std::printf("* = staged through host memory (no peer access); link numbers are invented\n");
    return 0;
}
