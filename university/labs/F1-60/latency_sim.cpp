// F1-60 Listing 1: a model of one SM's warp scheduler hiding memory latency.
// It is a model, not a real GPU. Rules of the model:
//   * at most one instruction issues per cycle (from any warp), in program order per warp;
//   * a load's data arrives `mem` cycles after it issues; a compute result ALU cycles later;
//   * each warp runs `iters` iterations; iteration k = one load, then `compute` dependent
//     compute instructions (the first needs load k's data);
//   * `ahead` = how many loads a warp issues in advance (0 = load, use, load, use ...).
// Input lines: label warps ahead compute mem iters timeline(0/1)
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

constexpr long ALU = 4;      // compute latency in cycles (TG-1 model value, invented)

struct Op
{
    bool load;               // true: load for iteration `it`; false: compute instruction
    int it;                  // iteration the op belongs to
    bool firstCompute;       // the compute op that waits for load `it`
};

std::vector<Op> program(int ahead, int compute, int iters)
{
    std::vector<Op> p;
    int nextLoad = 0;
    for (; nextLoad <= ahead && nextLoad < iters; ++nextLoad) { p.push_back({true, nextLoad, false}); }
    for (int k = 0; k < iters; ++k) {
        for (int c = 0; c < compute; ++c) { p.push_back({false, k, c == 0}); }
        if (nextLoad < iters) { p.push_back({true, nextLoad, false}); ++nextLoad; }
    }
    return p;
}

struct Result
{
    long cycles = 0, issued = 0, stallMem = 0, stallAlu = 0;
};

Result simulate(int warps, int ahead, int compute, long mem, int iters, bool timeline)
{
    const std::vector<Op> prog = program(ahead, compute, iters);
    const std::size_t nw = static_cast<std::size_t>(warps);
    std::vector<std::size_t> pc(nw, 0);
    std::vector<long> lastIssue(nw, -1000);
    std::vector<std::vector<long>> arrival(nw, std::vector<long>(static_cast<std::size_t>(iters), 0));
    std::vector<std::string> rows(nw);
    std::string issueRow;
    Result r;
    std::size_t last = nw - 1;
    auto readyAt = [&](std::size_t w) {
        const Op& op = prog[pc[w]];
        long t = lastIssue[w] + 1;                                   // in-order issue
        if (!op.load && op.firstCompute) { t = std::max(t, arrival[w][static_cast<std::size_t>(op.it)]); }
        if (!op.load && !op.firstCompute) { t = std::max(t, lastIssue[w] + ALU); }
        return t;
    };
    for (long cycle = 0;; ++cycle) {
        bool allDone = true;
        for (std::size_t w = 0; w < nw; ++w) { allDone = allDone && pc[w] == prog.size(); }
        if (allDone) { r.cycles = cycle; break; }
        std::size_t pick = nw;                                       // round robin over ready warps
        for (std::size_t k = 1; k <= nw; ++k) {
            std::size_t w = (last + k) % nw;
            if (pc[w] < prog.size() && readyAt(w) <= cycle) { pick = w; break; }
        }
        bool anyMem = false;
        for (std::size_t w = 0; w < nw; ++w) {
            char c = ' ';
            if (pc[w] == prog.size()) { c = ' '; }
            else if (w == pick) { c = prog[pc[w]].load ? 'L' : 'c'; }
            else if (readyAt(w) <= cycle) { c = 'r'; }
            else {
                bool memWait = !prog[pc[w]].load && prog[pc[w]].firstCompute &&
                               arrival[w][static_cast<std::size_t>(prog[pc[w]].it)] > cycle;
                c = memWait ? 'm' : 'a';
                anyMem = anyMem || memWait;
            }
            if (timeline && cycle < 100) { rows[w] += c; }
        }
        if (pick == nw) {
            if (anyMem) { ++r.stallMem; } else { ++r.stallAlu; }
            if (timeline && cycle < 100) { issueRow += '.'; }
            continue;
        }
        if (timeline && cycle < 100) { issueRow += static_cast<char>('0' + static_cast<int>(pick % 10)); }
        const Op& op = prog[pc[pick]];
        if (op.load) { arrival[pick][static_cast<std::size_t>(op.it)] = cycle + mem; }
        lastIssue[pick] = cycle;
        ++pc[pick];
        ++r.issued;
        last = pick;
    }
    if (timeline) {
        std::printf("  cycle    0         10        20        30        40        50        60        70        80        90\n");
        std::printf("  issue    %s\n", issueRow.c_str());
        for (std::size_t w = 0; w < nw; ++w) { std::printf("  warp %-3zu %s\n", w, rows[w].c_str()); }
    }
    return r;
}

int main()
{
    std::printf("model: 1 issue per cycle, compute latency %ld cycles\n", ALU);
    std::string label;
    int warps = 0, ahead = 0, compute = 0, iters = 0, timeline = 0;
    long mem = 0;
    while (std::cin >> label >> warps >> ahead >> compute >> mem >> iters >> timeline) {
        Result r = simulate(warps, ahead, compute, mem, iters, timeline != 0);
        double busy = 100.0 * static_cast<double>(r.issued) / static_cast<double>(r.cycles);
        std::printf("%-20s warps %2d ahead %d compute %d mem %3ld iters %2d | cycles %5ld issued %4ld busy %5.1f %% | "
                    "idle: memory %4ld, compute %3ld\n",
                    label.c_str(), warps, ahead, compute, mem, iters, r.cycles, r.issued, busy, r.stallMem, r.stallAlu);
    }
    return 0;
}
