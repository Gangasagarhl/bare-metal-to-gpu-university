// F7-16 Listing 3: a CPU replay of sgemmLdsReg (Listing 1), thread by thread, phase by phase.
// It checks the tiling and edge logic on the E6 shape list without a GPU.
// Mode 2 replays Kai's kernel (kai_kernel.hip, the forensic lab) with one legal schedule:
// wave 0 (threads 0..63) runs one phase ahead of waves 1..3.
#include <cmath>
#include <cstdio>
#include <vector>

constexpr int BM = 64, BN = 64, BK = 16, TM = 4, TN = 4, PAD = 4, THREADS = 256, WAVE = 64;

struct Lds { float As[BK][BM + PAD]; float Bs[BK][BN]; };

void loadTile(Lds& s, const std::vector<float>& A, const std::vector<float>& B, int M, int N, int K,
              int rowBase, int colBase, int k0, int tid)
{
    for (int i = tid; i < BM * BK; i += THREADS) {
        const int m = i / BK, k = i % BK, gr = rowBase + m, gk = k0 + k;
        s.As[k][m] = (gr < M && gk < K) ? A[gr * K + gk] : 0.0f;
    }
    for (int i = tid; i < BK * BN; i += THREADS) {
        const int k = i / BN, n = i % BN, gk = k0 + k, gc = colBase + n;
        s.Bs[k][n] = (gk < K && gc < N) ? B[gk * N + gc] : 0.0f;
    }
}

void compute(const Lds& s, float (&acc)[TM][TN], int tid)
{
    const int tRow = tid / 16, tCol = tid % 16;
    for (int k = 0; k < BK; ++k) {
        for (int i = 0; i < TM; ++i) {
            for (int j = 0; j < TN; ++j) {
                acc[i][j] += s.As[k][tRow * TM + i] * s.Bs[k][tCol * TN + j];
            }
        }
    }
}

std::vector<float> replay(const std::vector<float>& A, const std::vector<float>& B, int M, int N, int K,
                          bool secondBarrier)
{
    std::vector<float> C(static_cast<size_t>(M) * N, 0.0f);
    for (int by = 0; by * BM < M; ++by) {
        for (int bx = 0; bx * BN < N; ++bx) {
            Lds s{};
            static float acc[THREADS][TM][TN];
            for (auto& a : acc) for (auto& r : a) for (auto& v : r) v = 0.0f;
            bool wave0Loaded = false;              // wave 0 already loaded this k-tile (ran ahead)
            for (int k0 = 0; k0 < K; k0 += BK) {
                for (int tid = 0; tid < THREADS; ++tid) {      // phase 1: load
                    if (!(wave0Loaded && tid < WAVE)) {
                        loadTile(s, A, B, M, N, K, by * BM, bx * BN, k0, tid);
                    }
                }
                wave0Loaded = false;
                // first __syncthreads(): every load above is complete
                for (int tid = 0; tid < WAVE; ++tid) {           // phase 2: wave 0 computes
                    compute(s, acc[tid], tid);
                }
                if (!secondBarrier && k0 + BK < K) {             // Kai: wave 0 starts the next k-tile
                    for (int tid = 0; tid < WAVE; ++tid) {
                        loadTile(s, A, B, M, N, K, by * BM, bx * BN, k0 + BK, tid);
                    }
                    wave0Loaded = true;
                }
                for (int tid = WAVE; tid < THREADS; ++tid) {     // phase 2: waves 1..3 compute
                    compute(s, acc[tid], tid);
                }
                // second __syncthreads() (when present): nobody loads before everyone computed
            }
            for (int tid = 0; tid < THREADS; ++tid) {
                const int tRow = tid / 16, tCol = tid % 16;
                for (int i = 0; i < TM; ++i) {
                    for (int j = 0; j < TN; ++j) {
                        const int r = by * BM + tRow * TM + i, c = bx * BN + tCol * TN + j;
                        if (r < M && c < N) {
                            C[static_cast<size_t>(r) * N + c] = acc[tid][i][j];
                        }
                    }
                }
            }
        }
    }
    return C;
}

int main()
{
    struct Shape { int M, N, K; const char* what; };
    const Shape shapes[] = {
        {256, 256, 256, "square"},
        {300, 300, 300, "square, not a tile multiple"},
        {1024, 64, 64, "tall-skinny"},
        {64, 1024, 64, "short-wide"},
        {77, 129, 33, "odd sizes"},
        {100, 100, 16, "K = one k-tile"},
    };
    int failures = 0;
    for (int mode = 0; mode < 2; ++mode) {
        std::printf("== %s\n", mode == 0 ? "Listing 1 (sgemmLdsReg)" : "Kai's kernel, wave 0 one phase ahead of waves 1..3");
        for (const Shape& s : shapes) {
            std::vector<float> A(static_cast<size_t>(s.M) * s.K), B(static_cast<size_t>(s.K) * s.N);
            for (size_t i = 0; i < A.size(); ++i) A[i] = static_cast<float>(static_cast<int>(i * 37 % 101) - 50) / 50.0f;
            for (size_t i = 0; i < B.size(); ++i) B[i] = static_cast<float>(static_cast<int>(i * 53 % 89) - 44) / 44.0f;
            const std::vector<float> C = replay(A, B, s.M, s.N, s.K, mode == 0);
            int bad = 0;
            int badRowsInBlock[BM] = {};
            for (int r = 0; r < s.M; ++r) {
                for (int c = 0; c < s.N; ++c) {
                    double ref = 0.0, mag = 0.0;
                    for (int k = 0; k < s.K; ++k) {
                        const double p = static_cast<double>(A[r * s.K + k]) * B[k * s.N + c];
                        ref += p;
                        mag += std::fabs(p);
                    }
                    if (std::fabs(C[static_cast<size_t>(r) * s.N + c] - ref) > 2.0 * s.K * std::ldexp(1.0, -24) * mag) {
                        ++bad;
                        ++badRowsInBlock[r % BM];
                    }
                }
            }
            int firstBadRow = -1, lastBadRow = -1;
            for (int r = 0; r < BM; ++r) {
                if (badRowsInBlock[r] > 0) {
                    if (firstBadRow < 0) firstBadRow = r;
                    lastBadRow = r;
                }
            }
            std::printf("%-28s M=%-5d N=%-5d K=%-4d wrong: %d", s.what, s.M, s.N, s.K, bad);
            if (bad > 0) {
                std::printf(" (rows %d..%d of each 64-row block tile)", firstBadRow, lastBadRow);
            }
            std::printf("\n");
            if (mode == 0 && bad > 0) ++failures;
        }
    }
    return failures == 0 ? 0 : 1;
}
