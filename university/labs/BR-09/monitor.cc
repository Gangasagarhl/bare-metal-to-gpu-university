// monitor.cc: a tiny monitor for stats_cli. It reads one line per invocation,
//   exit=<status> <first line the program wrote to stderr, possibly empty>
// counts outcomes by error code, and raises an alert when the error rate is above a
// threshold or when any run crashed or failed without a known code.
//   usage: monitor <threshold percent>   (log on standard input)
#include <cstdio>
#include <iostream>
#include <map>
#include <regex>
#include <string>

int main(int argc, char** argv)
{
    const double threshold = argc > 1 ? std::stod(argv[1]) : 5.0;
    const std::regex line_re(R"(^exit=(\d+) ?(.*)$)");
    const std::regex code_re(R"(error (E-[A-Z]+))");
    std::map<std::string, long> by_outcome;
    long total = 0;
    long errors = 0;
    long unexplained = 0;
    std::string line;
    while (std::getline(std::cin, line)) {
        std::smatch m;
        if (!std::regex_match(line, m, line_re)) {
            continue;
        }
        ++total;
        const int status = std::stoi(m[1].str());
        const std::string text = m[2].str();
        if (status == 0) {
            ++by_outcome["ok (exit 0)"];
            continue;
        }
        ++errors;
        std::smatch c;
        if (std::regex_search(text, c, code_re)) {
            ++by_outcome[c[1].str() + " (exit " + std::to_string(status) + ")"];
        } else {
            ++unexplained;
            ++by_outcome["NO CODE (exit " + std::to_string(status) + ")"];
        }
    }
    std::printf("invocations: %ld\n", total);
    for (const auto& [outcome, n] : by_outcome) {
        std::printf("  %-22s %4ld\n", outcome.c_str(), n);
    }
    const double rate = total > 0 ? 100.0 * static_cast<double>(errors) / static_cast<double>(total)
                                  : 0.0;
    std::printf("error rate: %.1f %% (threshold %.1f %%)\n", rate, threshold);
    if (unexplained > 0) {
        std::printf("ALERT: %ld run(s) failed without an error code (crash?)\n", unexplained);
    }
    if (rate > threshold) {
        std::printf("ALERT: error rate above threshold\n");
    }
    if (unexplained == 0 && rate <= threshold) {
        std::printf("no alert\n");
    }
    return 0;
}
