// SPDX-License-Identifier: LicenseRef-Uni-Lab
// stats_cli: prints the count, mean and median of the numbers given on the command line.
// The output line and the exit codes are a public interface (README.md, "Interface"):
//   0 success · 2 E-USAGE · 3 E-PARSE · 4 E-RANGE · 5 E-INTERNAL
#include "uni/parse.h"
#include "uni/stats.h"

#include <cstdio>
#include <exception>
#include <string_view>
#include <variant>
#include <vector>

#ifndef UNI_VERSION
#error "UNI_VERSION must be set by the build (CMakeLists.txt passes PROJECT_VERSION)"
#endif

namespace {

constexpr int kUsage = 2;
constexpr int kParse = 3;
constexpr int kRange = 4;
constexpr int kInternal = 5;

int usage()
{
    std::fprintf(stderr, "stats_cli: error E-USAGE: no numbers given\n"
                         "usage: stats_cli <number>...   |   stats_cli --version\n");
    return kUsage;
}

int run(int argc, char** argv)
{
    if (argc == 2 && std::string_view(argv[1]) == "--version") {
        std::printf("stats_cli %s\n", UNI_VERSION);
        return 0;
    }
    std::vector<double> xs;
    for (int i = 1; i < argc; ++i) {
        const auto parsed = uni::parse_number(argv[i]);
        if (const auto* err = std::get_if<uni::ParseError>(&parsed)) {
            std::fprintf(stderr, "stats_cli: error E-PARSE: argument %d \"%s\" %s\n", i, argv[i],
                         uni::describe(*err));
            return kParse;
        }
        xs.push_back(std::get<double>(parsed));
    }
    if (xs.empty()) {
        return usage();
    }
    const auto m = uni::mean(xs);
    const auto md = uni::median(xs);
    if (!m || !md) {
        std::fprintf(stderr, "stats_cli: error E-RANGE: result does not fit in a double "
                             "(see KNOWN_ISSUES.md, KI-1)\n");
        return kRange;
    }
    std::printf("count %zu mean %g median %g\n", xs.size(), *m, *md);  // format kept from 0.1
    return 0;
}

}  // namespace

int main(int argc, char** argv)
{
    try {
        return run(argc, argv);
    } catch (const std::exception& e) {
        std::fprintf(stderr, "stats_cli: error E-INTERNAL: %s\n", e.what());
        return kInternal;
    }
}
