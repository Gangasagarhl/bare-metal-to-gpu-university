// F6-32 Listing 3: summarise a timeline export, the questions you ask of any
// timeline (Nsight Systems or this course's model): how busy is each lane, how
// long is the compute lane idle, and for how long do copies overlap kernels?
// Input: lines "csv: lane,start,end,stream,label" (other lines are ignored), as
// printed by the F6-28 model (Listing 2 of F6-28) with the "csv" command.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

struct Span
{
    std::string lane;
    double start;
    double end;
    std::string label;
};

int main()
{
    std::vector<Span> spans;
    std::string line;
    while (std::getline(std::cin, line)) {
        const auto at = line.find("csv: ");
        if (at == std::string::npos || line.find("lane,start") != std::string::npos) {
            continue;
        }
        std::istringstream in(line.substr(at + 5));
        std::string lane, start, end, stream, label;
        std::getline(in, lane, ',');
        std::getline(in, start, ',');
        std::getline(in, end, ',');
        std::getline(in, stream, ',');
        std::getline(in, label);
        spans.push_back({lane, std::stod(start), std::stod(end), label});
    }
    if (spans.empty()) {
        std::printf("no timeline rows found\n");
        return 1;
    }
    double t0 = spans[0].start;
    double t1 = spans[0].end;
    for (const Span& s : spans) {
        t0 = std::min(t0, s.start);
        t1 = std::max(t1, s.end);
    }
    const double total = t1 - t0;
    std::printf("timeline: %zu rows, %.1f to %.1f (total %.1f)\n", spans.size(), t0, t1, total);
    std::map<std::string, double> busy;
    for (const Span& s : spans) { busy[s.lane] += s.end - s.start; }
    for (const auto& [lane, b] : busy) {
        std::printf("  lane %-9s busy %7.1f  (%5.1f %%)\n", lane.c_str(), b, 100.0 * b / total);
    }
    std::vector<Span> compute;
    for (const Span& s : spans) {
        if (s.lane == "compute") { compute.push_back(s); }
    }
    std::sort(compute.begin(), compute.end(),
              [](const Span& a, const Span& b) { return a.start < b.start; });
    double idle = compute.empty() ? total : compute.front().start - t0;
    double longest = idle;
    for (std::size_t i = 1; i < compute.size(); ++i) {
        const double gap = compute[i].start - compute[i - 1].end;
        if (gap > 0) {
            idle += gap;
            longest = std::max(longest, gap);
        }
    }
    std::printf("  compute idle %.1f in total, longest gap %.1f\n", idle, longest);
    std::vector<double> cuts;
    for (const Span& s : spans) { cuts.push_back(s.start); cuts.push_back(s.end); }
    std::sort(cuts.begin(), cuts.end());
    double overlap = 0.0;
    for (std::size_t i = 0; i + 1 < cuts.size(); ++i) {
        const double mid = 0.5 * (cuts[i] + cuts[i + 1]);
        bool copy = false;
        bool kernel = false;
        for (const Span& s : spans) {
            if (s.start <= mid && mid < s.end) {
                if (s.lane == "compute") { kernel = true; }
                if (s.lane.rfind("copy", 0) == 0) { copy = true; }
            }
        }
        if (copy && kernel) { overlap += cuts[i + 1] - cuts[i]; }
    }
    std::printf("  copies overlapping kernels: %.1f (%.1f %% of total)\n", overlap,
                100.0 * overlap / total);
    return 0;
}
