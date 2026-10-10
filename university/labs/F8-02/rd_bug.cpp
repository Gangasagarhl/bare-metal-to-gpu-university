// F8-02 forensic evidence: "fine on 4 and 8 GPUs, wrong on 6".
// A recursive-doubling all-reduce written for powers of two. The planted bug: when the
// partner rank (rank XOR mask) does not exist, the rank silently skips that round.
// Each rank r contributes 10^r, so the decimal digits of a result say how many times each
// rank's data was added: a correct all-reduce gives the digit 1 for every rank.
#include <cstdio>
#include <string>
#include <vector>

std::vector<long> buggyAllReduce(int n)
{
    std::vector<long> v(static_cast<std::size_t>(n));
    long p = 1;
    for (int r = 0; r < n; ++r) {
        v[static_cast<std::size_t>(r)] = p;              // rank r contributes 10^r
        p *= 10;
    }
    for (int mask = 1; mask < n; mask <<= 1) {
        const std::vector<long> before = v;
        for (int r = 0; r < n; ++r) {
            const int partner = r ^ mask;
            if (partner < n) {                           // BUG: no plan for a missing partner
                v[static_cast<std::size_t>(r)] += before[static_cast<std::size_t>(partner)];
            }
        }
    }
    return v;
}

int main()
{
    bool allOk = true;
    for (int n = 2; n <= 8; ++n) {
        const std::vector<long> v = buggyAllReduce(n);
        int wrong = 0;
        std::string detail;
        for (int r = 0; r < n; ++r) {
            std::string digits;                          // contributions of ranks 0..n-1
            long x = v[static_cast<std::size_t>(r)];
            bool ok = true;
            for (int k = 0; k < n; ++k) {
                const long d = x % 10;
                x /= 10;
                digits += std::to_string(d);
                ok = ok && (d == 1);
            }
            if (!ok) {
                ++wrong;
                detail += "    rank " + std::to_string(r) + " holds contributions " + digits + "\n";
            }
        }
        std::printf("N = %d: %s", n, wrong == 0 ? "all ranks correct\n" : "");
        if (wrong != 0) {
            std::printf("%d of %d ranks WRONG (digit k = times rank k was added)\n%s", wrong, n, detail.c_str());
            allOk = false;
        }
    }
    std::printf("summary: %s\n", allOk ? "correct for every N" : "wrong for some N (see above)");
    return 0;   // the evidence is the printout; the program itself ran as intended
}
