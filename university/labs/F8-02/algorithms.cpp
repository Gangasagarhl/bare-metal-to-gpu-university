// F8-02 Listing 1: four all-reduce algorithms on simulated ranks.
// Every algorithm is a list of rounds; in one round each rank sends at most one message and
// receives at most one. A message carries a range of elements and is either added into the
// receiver's buffer (reduce) or copied over it (gather). The simulator applies the rounds,
// checks the result against the reference sum, and counts rounds and elements sent.
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

struct Msg
{
    int src;
    int dst;
    int offset;     // first element
    int length;     // number of elements
    bool add;       // true: dst += src (reduce); false: dst = src (gather)
};
using Round = std::vector<Msg>;
using Schedule = std::vector<Round>;

int segStart(int seg, int n, int count) { return seg * count / n; }   // uneven sizes allowed
int segLen(int seg, int n, int count) { return segStart(seg + 1, n, count) - segStart(seg, n, count); }
int mod(int a, int n) { return ((a % n) + n) % n; }

// Ring: N-1 reduce-scatter rounds, then N-1 all-gather rounds; segment = count / N elements
Schedule ring(int n, int count)
{
    Schedule s;
    for (int step = 0; step < n - 1; ++step) {           // reduce-scatter
        Round r;
        for (int rank = 0; rank < n; ++rank) {
            const int seg = mod(rank - step, n);
            r.push_back({rank, mod(rank + 1, n), segStart(seg, n, count), segLen(seg, n, count), true});
        }
        s.push_back(r);
    }
    for (int step = 0; step < n - 1; ++step) {           // all-gather
        Round r;
        for (int rank = 0; rank < n; ++rank) {
            const int seg = mod(rank + 1 - step, n);
            r.push_back({rank, mod(rank + 1, n), segStart(seg, n, count), segLen(seg, n, count), false});
        }
        s.push_back(r);
    }
    return s;
}

// Recursive doubling (N a power of two): log2(N) rounds, partners exchange the whole buffer
Schedule recursiveDoubling(int n, int count)
{
    Schedule s;
    for (int mask = 1; mask < n; mask <<= 1) {
        Round r;
        for (int rank = 0; rank < n; ++rank) {
            r.push_back({rank, rank ^ mask, 0, count, true});
        }
        s.push_back(r);
    }
    return s;
}

// Recursive halving then doubling (Rabenseifner; N a power of two): each round exchanges half
// of the current range, so the volume per rank matches the ring with only 2*log2(N) rounds
Schedule halvingDoubling(int n, int count)
{
    Schedule s;
    std::vector<int> lo(static_cast<std::size_t>(n), 0), len(static_cast<std::size_t>(n), count);
    std::vector<std::vector<int>> loHist, lenHist;
    for (int mask = n / 2; mask >= 1; mask /= 2) {       // reduce-scatter by halving
        loHist.push_back(lo);
        lenHist.push_back(len);
        Round r;
        std::vector<int> newLo = lo, newLen = len;
        for (int rank = 0; rank < n; ++rank) {
            const auto i = static_cast<std::size_t>(rank);
            const int half = len[i] / 2;
            const bool keepLow = (rank & mask) == 0;     // lower rank keeps the lower half
            const int sendLo = keepLow ? lo[i] + half : lo[i];
            const int sendLen = keepLow ? len[i] - half : half;
            r.push_back({rank, rank ^ mask, sendLo, sendLen, true});
            newLo[i] = keepLow ? lo[i] : lo[i] + half;
            newLen[i] = keepLow ? half : len[i] - half;
        }
        s.push_back(r);
        lo = newLo;
        len = newLen;
    }
    for (int mask = 1; mask < n; mask *= 2) {            // all-gather by doubling, in reverse
        Round r;
        for (int rank = 0; rank < n; ++rank) {
            const auto i = static_cast<std::size_t>(rank);
            r.push_back({rank, rank ^ mask, lo[i], len[i], false});
        }
        s.push_back(r);
        const auto& prevLo = loHist.back();
        const auto& prevLen = lenHist.back();
        lo = prevLo;
        len = prevLen;
        loHist.pop_back();
        lenHist.pop_back();
    }
    return s;
}

// Binomial tree: reduce to rank 0 in ceil(log2 N) rounds, then broadcast back the same way
Schedule binomialTree(int n, int count)
{
    Schedule s;
    int levels = 0;
    while ((1 << levels) < n) {
        ++levels;
    }
    for (int k = 0; k < levels; ++k) {                   // reduce: rank with bit k set sends down
        Round r;
        for (int rank = 0; rank < n; ++rank) {
            const int low = (1 << (k + 1)) - 1;
            if ((rank & low) == (1 << k)) {
                r.push_back({rank, rank - (1 << k), 0, count, true});
            }
        }
        s.push_back(r);
    }
    for (int k = levels - 1; k >= 0; --k) {              // broadcast: the reverse tree, copying
        Round r;
        for (int rank = 0; rank < n; ++rank) {
            const int low = (1 << (k + 1)) - 1;
            if ((rank & low) == 0 && rank + (1 << k) < n) {
                r.push_back({rank, rank + (1 << k), 0, count, false});
            }
        }
        s.push_back(r);
    }
    return s;
}

struct Result
{
    bool correct;
    int rounds;
    long maxSent;       // elements sent by the busiest rank
    long totalSent;     // elements sent by all ranks
};

Result simulate(const Schedule& sched, int n, int count)
{
    std::vector<std::vector<long>> buf(static_cast<std::size_t>(n), std::vector<long>(static_cast<std::size_t>(count)));
    std::vector<long> ref(static_cast<std::size_t>(count), 0);
    for (int r = 0; r < n; ++r) {
        for (int i = 0; i < count; ++i) {
            const long v = 1000L * (r + 1) + i;          // distinct values per rank and element
            buf[static_cast<std::size_t>(r)][static_cast<std::size_t>(i)] = v;
            ref[static_cast<std::size_t>(i)] += v;
        }
    }
    std::vector<long> sent(static_cast<std::size_t>(n), 0);
    for (const Round& round : sched) {
        const auto before = buf;                         // all messages of a round leave at once
        for (const Msg& m : round) {
            for (int i = m.offset; i < m.offset + m.length; ++i) {
                const long v = before[static_cast<std::size_t>(m.src)][static_cast<std::size_t>(i)];
                long& d = buf[static_cast<std::size_t>(m.dst)][static_cast<std::size_t>(i)];
                d = m.add ? d + v : v;
            }
            sent[static_cast<std::size_t>(m.src)] += m.length;
        }
    }
    bool ok = true;
    for (const auto& b : buf) {
        ok = ok && (b == ref);
    }
    long total = 0;
    for (long x : sent) {
        total += x;
    }
    return {ok, static_cast<int>(sched.size()), *std::max_element(sent.begin(), sent.end()), total};
}

// Trace of the ring for N = 4: which ranks' contributions each segment holds after each round
void traceRing()
{
    const int n = 4;
    std::vector<std::vector<int>> mask(n, std::vector<int>(n));   // mask[rank][segment]
    for (int r = 0; r < n; ++r) {
        for (int g = 0; g < n; ++g) {
            mask[static_cast<std::size_t>(r)][static_cast<std::size_t>(g)] = 1 << r;
        }
    }
    auto print = [&](const std::string& title) {
        std::printf("%s\n", title.c_str());
        for (int r = 0; r < n; ++r) {
            std::string line = "  rank " + std::to_string(r) + ":";
            for (int g = 0; g < n; ++g) {
                std::string who;
                for (int k = 0; k < n; ++k) {
                    if (mask[static_cast<std::size_t>(r)][static_cast<std::size_t>(g)] & (1 << k)) {
                        who += std::to_string(k);
                    }
                }
                line += "  seg" + std::to_string(g) + "=" + who + std::string(5 - who.size(), ' ');
            }
            std::printf("%s\n", line.c_str());
        }
    };
    print("ring, N = 4: digits = ranks whose data a segment contains; start:");
    const Schedule s = ring(n, n);                       // one element per segment
    int idx = 0;
    for (const Round& round : s) {
        const auto before = mask;
        std::string moves;
        for (const Msg& m : round) {
            int& d = mask[static_cast<std::size_t>(m.dst)][static_cast<std::size_t>(m.offset)];
            const int v = before[static_cast<std::size_t>(m.src)][static_cast<std::size_t>(m.offset)];
            d = m.add ? (d | v) : v;
            moves += " " + std::to_string(m.src) + "->" + std::to_string(m.dst) + ":seg" + std::to_string(m.offset);
        }
        const bool rs = idx < n - 1;
        const int stepNo = rs ? idx + 1 : idx - (n - 1) + 1;
        print(std::string(rs ? "after reduce-scatter step " : "after all-gather step ") +
              std::to_string(stepNo) + " (" + (rs ? "add" : "copy") + "):" + moves);
        ++idx;
    }
}

int main()
{
    traceRing();
    const int count = 240;                               // elements; divisible by 2, 3, 4, 8, 16
    std::printf("\nall-reduce of %d elements per rank; 'sent' = elements sent by the busiest rank\n", count);
    std::printf("%4s  %-20s %7s %8s %10s  %s\n", "N", "algorithm", "rounds", "sent", "sent/count", "result");
    bool allOk = true;
    for (int n : {2, 3, 4, 8, 16}) {
        const bool pow2 = (n & (n - 1)) == 0;
        struct Alg { const char* name; Schedule s; bool valid; };
        const std::vector<Alg> algs = {
            {"ring", ring(n, count), true},
            {"recursive doubling", pow2 ? recursiveDoubling(n, count) : Schedule{}, pow2},
            {"halving-doubling", pow2 ? halvingDoubling(n, count) : Schedule{}, pow2},
            {"binomial tree", binomialTree(n, count), true},
        };
        for (const Alg& a : algs) {
            if (!a.valid) {
                std::printf("%4d  %-20s %7s %8s %10s  %s\n", n, a.name, "-", "-", "-", "(needs a power of two)");
                continue;
            }
            const Result r = simulate(a.s, n, count);
            allOk = allOk && r.correct;
            std::printf("%4d  %-20s %7d %8ld %10.3f  %s\n", n, a.name, r.rounds, r.maxSent,
                        static_cast<double>(r.maxSent) / count, r.correct ? "correct" : "WRONG");
        }
    }
    std::printf("formulas: ring rounds 2(N-1), sent/count 2(N-1)/N; recursive doubling rounds log2 N,"
                " sent/count log2 N\n");
    return allOk ? 0 : 1;
}
