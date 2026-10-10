// F6-26 Listing 3: does the block -> warp -> fragment walk of gemmWmma write every element
// of C exactly once, and read only inside A and B? The same index arithmetic as Listing 1,
// run on the CPU for the launch the host code makes (grid of 64 x 64 tiles, 4 warps each).
#include <cstdio>
#include <vector>

int main()
{
    const int WM = 16, WN = 16, WK = 16, WARP_TILE = 32, BLOCK_TILE = 64;
    const int shapes[][3] = {{16, 16, 16}, {48, 80, 32}, {64, 64, 64}, {112, 208, 96},
                             {256, 16, 512}, {4096, 4096, 4096}};
    for (const auto& s : shapes) {
        const int M = s[0], N = s[1], K = s[2];
        std::vector<int> written(size_t(M) * N, 0);
        long outsideA = 0, outsideB = 0, mmas = 0;
        const int gridX = (N + BLOCK_TILE - 1) / BLOCK_TILE, gridY = (M + BLOCK_TILE - 1) / BLOCK_TILE;
        for (int by = 0; by < gridY; ++by) {
            for (int bx = 0; bx < gridX; ++bx) {
                for (int warp = 0; warp < 4; ++warp) {
                    const int warpRow = by * BLOCK_TILE + (warp / 2) * WARP_TILE;
                    const int warpCol = bx * BLOCK_TILE + (warp % 2) * WARP_TILE;
                    if (warpRow >= M || warpCol >= N) {
                        continue;
                    }
                    for (int k = 0; k < K; k += WK) {
                        for (int i = 0; i < 2; ++i) {      // A fragment: rows r..r+15, cols k..k+15
                            const int r = warpRow + i * WM;
                            if (r < M && (r + WM > M || k + WK > K)) {
                                ++outsideA;
                            }
                        }
                        for (int j = 0; j < 2; ++j) {
                            const int c = warpCol + j * WN;
                            if (c < N && (c + WN > N || k + WK > K)) {
                                ++outsideB;
                            }
                        }
                        for (int i = 0; i < 2; ++i) {
                            for (int j = 0; j < 2; ++j) {
                                mmas += (warpRow + i * WM < M && warpCol + j * WN < N) ? 1 : 0;
                            }
                        }
                    }
                    for (int i = 0; i < 2; ++i) {
                        for (int j = 0; j < 2; ++j) {
                            const int r = warpRow + i * WM, c = warpCol + j * WN;
                            if (r < M && c < N) {
                                for (int y = 0; y < WM; ++y) {
                                    for (int x = 0; x < WN; ++x) {
                                        ++written[size_t(r + y) * N + (c + x)];
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
        long once = 0, other = 0;
        for (int w : written) {
            (w == 1 ? once : other) += 1;
        }
        std::printf("M=%5d N=%5d K=%5d: %ld elements written once, %ld not once; fragment loads "
                    "outside A: %ld, outside B: %ld; mma_sync calls %ld (= M*N*K/4096: %s)\n",
                    M, N, K, once, other, outsideA, outsideB, mmas,
                    mmas == long(M) * N * K / 4096 ? "yes" : "no");
    }
    return 0;
}
