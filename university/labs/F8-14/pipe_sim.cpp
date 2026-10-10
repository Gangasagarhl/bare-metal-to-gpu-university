// F8-14 Listing 1: pipeline schedules on paper, computed. S stages, M micro-batches; a forward
// step takes tf time units on every stage and a backward step tb. F(s,m) needs F(s-1,m);
// B(s,m) needs B(s+1,m), and on the last stage F(S-1,m). Communication is free in this model.
// Schedules: "gpipe" (all forwards, then all backwards) and "1f1b" (warm-up forwards, then one
// forward and one backward in turn, then the remaining backwards).
// Input lines: S M tf tb schedule. Output: a timeline per stage (A, B, C... = forward of
// micro-batch 0, 1, 2...; a, b, c... = backward; '.' = idle), the bubble fraction and the
// largest number of micro-batches whose activations a stage holds at once.
#include <algorithm>
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

struct Op
{
    bool fwd;
    int mb;
};

std::vector<Op> order(int s, int S, int M, const std::string& sched)
{
    std::vector<Op> ops;
    if (sched == "gpipe") {
        for (int m = 0; m < M; ++m) ops.push_back({true, m});
        for (int m = 0; m < M; ++m) ops.push_back({false, m});
        return ops;
    }
    const int warm = std::min(S - 1 - s, M);       // 1f1b
    int f = 0, b = 0;
    for (; f < warm; ++f) ops.push_back({true, f});
    while (f < M) {
        ops.push_back({true, f++});
        ops.push_back({false, b++});
    }
    while (b < M) ops.push_back({false, b++});
    return ops;
}

int main()
{
    int S, M, tf, tb;
    std::string sched;
    while (std::cin >> S >> M >> tf >> tb >> sched) {
        std::vector<std::vector<int>> fEnd(S, std::vector<int>(M, -1)), bEnd = fEnd;
        std::vector<std::vector<Op>> ops(S);
        for (int s = 0; s < S; ++s) ops[s] = order(s, S, M, sched);
        std::vector<std::size_t> next(S, 0);
        std::vector<int> free(S, 0), held(S, 0), peak(S, 0);
        std::vector<std::string> line(S);
        bool progress = true;
        while (progress) {                          // repeatedly start every op whose inputs exist
            progress = false;
            for (int s = 0; s < S; ++s) {
                if (next[s] == ops[s].size()) continue;
                const Op op = ops[s][next[s]];
                int ready;
                if (op.fwd) {
                    ready = s == 0 ? 0 : fEnd[s - 1][op.mb];
                } else {
                    ready = s == S - 1 ? fEnd[s][op.mb] : bEnd[s + 1][op.mb];
                }
                if (ready < 0) continue;
                const int start = std::max(ready, free[s]);
                const int len = op.fwd ? tf : tb;
                line[s].resize(start, '.');
                line[s].append(len, static_cast<char>((op.fwd ? 'A' : 'a') + op.mb % 26));
                free[s] = start + len;
                (op.fwd ? fEnd : bEnd)[s][op.mb] = free[s];
                held[s] += op.fwd ? 1 : -1;
                peak[s] = std::max(peak[s], held[s]);
                ++next[s];
                progress = true;
            }
        }
        const int span = *std::max_element(free.begin(), free.end());
        const double busy = static_cast<double>(S) * M * (tf + tb);
        std::printf("%s  S = %d stages, M = %d micro-batches, tf = %d, tb = %d\n", sched.c_str(), S, M, tf, tb);
        for (int s = 0; s < S; ++s) {
            line[s].resize(span, '.');
            if (span <= 100) std::printf("  stage %d |%s|  holds at most %d\n", s, line[s].c_str(), peak[s]);
            else std::printf("  stage %d (timeline longer than 100 units, not drawn)  holds at most %d\n", s, peak[s]);
        }
        std::printf("  time %d; ideal %d; bubble fraction %.3f; formula (S-1)/(M+S-1) = %.3f\n\n",
                    span, M * (tf + tb), 1.0 - busy / (static_cast<double>(S) * span),
                    static_cast<double>(S - 1) / (M + S - 1));
    }
    return 0;
}
