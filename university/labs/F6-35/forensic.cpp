// F6-35 forensic evidence generator: a 2 x 2 block of a 4 x 6 row-major matrix A multiplied by a
// 2 x 2 matrix B, both through the column-major convention with swapped operands (Listing 1).
// The sub-block starts at row 1, column 2 of A. Build "good" passes the leading dimension of the
// FULL matrix (6); build "bad" passes the width of the block (2). The log prints the arguments.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

void sgemmColMajor(int m, int n, int k, const float* a, int lda, const float* b, int ldb, float* c, int ldc)
{
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            float acc = 0.0f;
            for (int p = 0; p < k; ++p) {
                acc += a[i + p * lda] * b[p + j * ldb];
            }
            c[i + j * ldc] = acc;
        }
    }
}

int main()
{
    std::vector<float> a(24);
    for (int i = 0; i < 24; ++i) {
        a[static_cast<std::size_t>(i)] = static_cast<float>(i);   // A[r][c] = 6r + c
    }
    const std::vector<float> b = {1, 0,
                                  0, 1};                            // identity: C must equal the block
    std::string build;
    while (std::cin >> build) {
        const int ldBlock = (build == "good") ? 6 : 2;
        const float* block = a.data() + 1 * 6 + 2;                   // &A[1][2]
        std::vector<float> c(4, -1.0f);
        // row-major C (2x2) = block (2x2) * B (2x2)  ->  column-major call C^T = B^T * block^T
        sgemmColMajor(2, 2, 2, b.data(), 2, block, ldBlock, c.data(), 2);
        std::printf("build %s: gemm(N, N, m=2, n=2, k=2, A=B, lda=2, B=&A[1][2], ldb=%d, ldc=2)\n",
                    build.c_str(), ldBlock);
        std::printf("  expected block: %4.0f %4.0f / %4.0f %4.0f\n", 8.0, 9.0, 14.0, 15.0);
        std::printf("  result C      : %4.0f %4.0f / %4.0f %4.0f\n", static_cast<double>(c[0]),
                    static_cast<double>(c[1]), static_cast<double>(c[2]), static_cast<double>(c[3]));
    }
    return 0;
}
