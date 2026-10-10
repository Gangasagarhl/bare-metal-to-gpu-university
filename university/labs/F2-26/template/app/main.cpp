// stats_cli: prints the count, mean and median of the numbers given on the command line.
#include "uni/stats.h"

#include <cstdio>
#include <string>
#include <vector>

int main(int argc, char** argv)
{
    std::vector<double> xs;
    for (int i = 1; i < argc; ++i) {
        xs.push_back(std::stod(argv[i]));
    }
    const auto m = uni::mean(xs);
    const auto md = uni::median(xs);
    if (!m || !md) {
        std::fprintf(stderr, "usage: stats_cli <number>...\n");
        return 2;
    }
    std::printf("count %zu mean %g median %g\n", xs.size(), *m, *md);
    return 0;
}
