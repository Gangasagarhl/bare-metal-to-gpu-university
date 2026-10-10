// MP4 Listing 2: the tile algorithm of the MP3 SGEMM, written once for three users:
// the AMD build, the NVIDIA build and the CPU emulator build of sgemm_kernel.hpp.
// C = A * B, row-major, any M, N, K. One workgroup computes a BM x BN tile of C; each of its
// THREADS work-items keeps a TM x TN sub-tile in registers. A tile is stored transposed (As[k][m]).
// The functions below are the work of ONE work-item between two barriers.
#pragma once
#include "mp4_portable.hpp"

template <int BM_, int BN_, int BK_, int TM_, int TN_, int PAD_>
struct TileCfg
{
    static constexpr int BM = BM_;   // rows of C per workgroup
    static constexpr int BN = BN_;   // columns of C per workgroup
    static constexpr int BK = BK_;   // depth of one LDS stage
    static constexpr int TM = TM_;   // rows of C per work-item
    static constexpr int TN = TN_;   // columns of C per work-item
    static constexpr int PAD = PAD_; // extra floats per row of As (bank shift, alignment)
    static constexpr int TCOLS = BN / TN;                 // work-items across a tile row
    static constexpr int THREADS = (BM / TM) * (BN / TN); // work-items per workgroup
    static_assert(BM % TM == 0 && BN % TN == 0, "a work-item tile must divide the block tile");
    static constexpr int ldsBytes()
    {
        return static_cast<int>(sizeof(float)) * (BK * (BM + PAD) + BK * BN);
    }
};

// Phase 1: copy one BK-deep stage of A and B from global memory into LDS; zero outside the matrix.
// The loop stride is Cfg::THREADS, never a literal: instances differ in work-items per workgroup.
template <class Cfg>
MP4_HD inline void loadStage(int tid, int k0, int rowBase, int colBase,
                             const float* A, const float* B, int M, int N, int K,
                             float (*As)[Cfg::BM + Cfg::PAD], float (*Bs)[Cfg::BN])
{
    for (int i = tid; i < Cfg::BM * Cfg::BK; i += Cfg::THREADS) {
        const int m = i / Cfg::BK;
        const int k = i % Cfg::BK;
        const int gr = rowBase + m;
        const int gk = k0 + k;
        As[k][m] = (gr < M && gk < K) ? A[gr * K + gk] : 0.0f;
    }
    for (int i = tid; i < Cfg::BK * Cfg::BN; i += Cfg::THREADS) {
        const int k = i / Cfg::BN;
        const int n = i % Cfg::BN;
        const int gk = k0 + k;
        const int gc = colBase + n;
        Bs[k][n] = (gk < K && gc < N) ? B[gk * N + gc] : 0.0f;
    }
}

// Phase 2: multiply the stage held in LDS into this work-item's register tile.
template <class Cfg>
MP4_HD inline void computeStage(int tid, const float (*As)[Cfg::BM + Cfg::PAD],
                                const float (*Bs)[Cfg::BN], float (&acc)[Cfg::TM][Cfg::TN])
{
    const int tRow = tid / Cfg::TCOLS;
    const int tCol = tid % Cfg::TCOLS;
    for (int k = 0; k < Cfg::BK; ++k) {
        float a[Cfg::TM];
        float b[Cfg::TN];
        for (int i = 0; i < Cfg::TM; ++i) {
            a[i] = As[k][tRow * Cfg::TM + i];
        }
        for (int j = 0; j < Cfg::TN; ++j) {
            b[j] = Bs[k][tCol * Cfg::TN + j];
        }
        for (int i = 0; i < Cfg::TM; ++i) {
            for (int j = 0; j < Cfg::TN; ++j) {
                acc[i][j] += a[i] * b[j];   // TM*TN FMAs for TM+TN LDS reads
            }
        }
    }
}

// Phase 3: write the register tile to C, skipping elements outside the matrix.
template <class Cfg>
MP4_HD inline void storeTile(int tid, int rowBase, int colBase,
                             const float (&acc)[Cfg::TM][Cfg::TN], float* C, int M, int N)
{
    const int tRow = tid / Cfg::TCOLS;
    const int tCol = tid % Cfg::TCOLS;
    for (int i = 0; i < Cfg::TM; ++i) {
        for (int j = 0; j < Cfg::TN; ++j) {
            const int r = rowBase + tRow * Cfg::TM + i;
            const int c = colBase + tCol * Cfg::TN + j;
            if (r < M && c < N) {
                C[r * N + c] = acc[i][j];
            }
        }
    }
}
