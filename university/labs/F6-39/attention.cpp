// F6-39 Listing 1: attention forward O = softmax(Q K^T / sqrt(d)) V for one head, two ways.
// naive: builds the whole N x N score matrix S and probability matrix P (as separate kernels
// would, through global memory). tiled: walks over K and V in tiles of Bc rows for each tile of
// Br query rows, keeping a running max m, normaliser l and unnormalised output in "registers"
// (the online softmax of F6-38), so S and P never exist in full.
// Masking: keys j < pad are padding (left-padded sequence); causal = 1 also hides keys j > i.
// guard = 1 skips the rescale while a row has seen only masked keys (m still -inf).
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

using Mat = std::vector<float>;   // row-major, rows x d
const float kNegInf = -std::numeric_limits<float>::infinity();

struct Traffic
{
    long long reads = 0;    // floats read from "global memory"
    long long writes = 0;   // floats written to "global memory"
    int badRows = 0;        // tiled only: rows whose final normaliser l is NaN or infinite
};

bool visible(int i, int j, int pad, bool causal)
{
    return j >= pad && (!causal || j <= i);
}

Mat naive(const Mat& q, const Mat& k, const Mat& v, int n, int d, int pad, bool causal, Traffic& t)
{
    const float scale = 1.0f / std::sqrt(static_cast<float>(d));
    Mat s(static_cast<std::size_t>(n) * n);
    for (int i = 0; i < n; ++i) {                     // kernel 1: S = Q K^T (masked)
        for (int j = 0; j < n; ++j) {
            float acc = 0.0f;
            for (int c = 0; c < d; ++c) {
                acc += q[i * d + c] * k[j * d + c];
            }
            s[i * n + j] = visible(i, j, pad, causal) ? acc * scale : kNegInf;
        }
    }
    t.reads += 2LL * n * d;                          // Q and K read once each (ideal reuse)
    t.writes += 1LL * n * n;
    for (int i = 0; i < n; ++i) {                     // kernel 2: P = softmax(S) row by row
        float m = kNegInf;
        for (int j = 0; j < n; ++j) {
            m = std::max(m, s[i * n + j]);
        }
        float l = 0.0f;
        for (int j = 0; j < n; ++j) {
            const float p = (m == kNegInf) ? 0.0f : std::exp(s[i * n + j] - m);
            s[i * n + j] = p;
            l += p;
        }
        for (int j = 0; j < n; ++j) {
            s[i * n + j] = (l > 0.0f) ? s[i * n + j] / l : 0.0f;
        }
    }
    t.reads += 1LL * n * n;
    t.writes += 1LL * n * n;
    Mat o(static_cast<std::size_t>(n) * d, 0.0f);   // kernel 3: O = P V
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int c = 0; c < d; ++c) {
                o[i * d + c] += s[i * n + j] * v[j * d + c];
            }
        }
    }
    t.reads += 1LL * n * n + 1LL * n * d;
    t.writes += 1LL * n * d;
    return o;
}

Mat tiled(const Mat& q, const Mat& k, const Mat& v, int n, int d, int br, int bc, int pad,
          bool causal, bool guard, Traffic& t)
{
    const float scale = 1.0f / std::sqrt(static_cast<float>(d));
    Mat o(static_cast<std::size_t>(n) * d, 0.0f);
    for (int i0 = 0; i0 < n; i0 += br) {             // one thread block per query tile
        const int rows = std::min(br, n - i0);
        t.reads += 1LL * rows * d;                    // Q tile loaded once
        std::vector<float> m(rows, kNegInf);
        std::vector<float> l(rows, 0.0f);
        std::vector<float> acc(static_cast<std::size_t>(rows) * d, 0.0f);
        for (int j0 = 0; j0 < n; j0 += bc) {         // stream K and V tiles through shared memory
            const int cols = std::min(bc, n - j0);
            t.reads += 2LL * cols * d;
            for (int r = 0; r < rows; ++r) {
                const int i = i0 + r;
                std::vector<float> sc(cols);
                float tileMax = kNegInf;
                for (int c = 0; c < cols; ++c) {
                    const int j = j0 + c;
                    float dot = 0.0f;
                    for (int x = 0; x < d; ++x) {
                        dot += q[i * d + x] * k[j * d + x];
                    }
                    sc[c] = visible(i, j, pad, causal) ? dot * scale : kNegInf;
                    tileMax = std::max(tileMax, sc[c]);
                }
                const float mNew = std::max(m[r], tileMax);
                if (guard && mNew == kNegInf) {
                    continue;                         // nothing visible yet: keep the empty state
                }
                const float alpha = std::exp(m[r] - mNew);   // rescale factor for the old state
                float rowSum = 0.0f;
                for (int x = 0; x < d; ++x) {
                    acc[r * d + x] *= alpha;
                }
                for (int c = 0; c < cols; ++c) {
                    const float p = std::exp(sc[c] - mNew);
                    rowSum += p;
                    for (int x = 0; x < d; ++x) {
                        acc[r * d + x] += p * v[(j0 + c) * d + x];
                    }
                }
                l[r] = l[r] * alpha + rowSum;
                m[r] = mNew;
            }
        }
        for (int r = 0; r < rows; ++r) {              // divide once at the end
            t.badRows += std::isfinite(l[r]) ? 0 : 1;
            for (int x = 0; x < d; ++x) {
                o[(i0 + r) * d + x] = (l[r] > 0.0f) ? acc[r * d + x] / l[r] : 0.0f;
            }
        }
        t.writes += 1LL * rows * d;
    }
    return o;
}

int main()
{
    std::string name;
    int n = 0;
    int d = 0;
    int br = 0;
    int bc = 0;
    int pad = 0;
    int causal = 0;
    int guard = 0;
    while (std::cin >> name >> n >> d >> br >> bc >> pad >> causal >> guard) {
        if (n <= 0 || d <= 0 || br <= 0 || bc <= 0 || pad < 0) {
            std::printf("%s: invalid input\n", name.c_str());
            return 1;
        }
        Mat q(static_cast<std::size_t>(n) * d);
        Mat k(q.size());
        Mat v(q.size());
        for (std::size_t i = 0; i < q.size(); ++i) {  // deterministic, varied values
            q[i] = static_cast<float>(static_cast<int>((i * 37) % 23) - 11) * 0.15f;
            k[i] = static_cast<float>(static_cast<int>((i * 53) % 19) - 9) * 0.2f;
            v[i] = static_cast<float>(static_cast<int>((i * 29) % 17) - 8) * 0.1f;
        }
        Traffic tn;
        Traffic tt;
        const Mat a = naive(q, k, v, n, d, pad, causal != 0, tn);
        const Mat b = tiled(q, k, v, n, d, br, bc, pad, causal != 0, guard != 0, tt);
        double err = 0.0;
        int nans = 0;
        int firstNanRow = -1;
        int zeroRows = 0;
        for (int i = 0; i < n; ++i) {
            bool allZero = true;
            for (int x = 0; x < d; ++x) {
                allZero = allZero && b[static_cast<std::size_t>(i) * d + x] == 0.0f;
            }
            zeroRows += allZero ? 1 : 0;
        }
        for (std::size_t i = 0; i < a.size(); ++i) {
            if (std::isnan(b[i])) {
                if (firstNanRow < 0) {
                    firstNanRow = static_cast<int>(i) / d;
                }
                ++nans;
                continue;
            }
            err = std::max(err, static_cast<double>(std::fabs(a[i] - b[i])));
        }
        std::printf("%s: N %d, d %d, tiles %dx%d, pad %d, causal %d, guard %d\n", name.c_str(), n, d,
                    br, bc, pad, causal, guard);
        std::printf("  max |naive - tiled| = %.3g over non-NaN outputs; NaN outputs: %d", err, nans);
        if (nans > 0) {
            std::printf(" (first in row %d)", firstNanRow);
        }
        std::printf("\n  tiled: rows with non-finite l: %d; rows of all zeros: %d", tt.badRows, zeroRows);
        std::printf("\n  floats moved: naive %lld read + %lld written; tiled %lld read + %lld written\n",
                    tn.reads, tn.writes, tt.reads, tt.writes);
    }
    return 0;
}
