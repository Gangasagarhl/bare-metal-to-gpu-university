// F7-19 Listing 1: calling a column-major BLAS-style GEMM with row-major data, without copies.
// A row-major M x K matrix, read as column-major, is its K x M transpose. So instead of
// C = A * B (row-major) we ask the column-major routine for C^T = B^T * A^T: pass B first,
// swap M and N, and use the row lengths as leading dimensions.
#include <cmath>
#include <cstdio>
#include <vector>

// A small column-major SGEMM with the BLAS argument order (no transposes, alpha and beta):
// C(m x n) = alpha * A(m x k) * B(k x n) + beta * C, element (i, j) of X at X[i + j * ldx].
void sgemmColMajor(int m, int n, int k, float alpha, const float* A, int lda, const float* B, int ldb,
                   float beta, float* C, int ldc)
{
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            float acc = 0.0f;
            for (int p = 0; p < k; ++p) {
                acc += A[i + p * lda] * B[p + j * ldb];
            }
            C[i + j * ldc] = alpha * acc + beta * C[i + j * ldc];
        }
    }
}

int main()
{
    const int M = 3, N = 4, K = 2;
    std::vector<float> A = {1, 2,      // row-major 3 x 2
                            3, 4,
                            5, 6};
    std::vector<float> B = {1, 0, 2, 1,     // row-major 2 x 4
                            0, 1, 1, 3};
    std::vector<float> C(M * N, 0.0f);
    // row-major C = A * B  ==  column-major C^T = B^T * A^T: m = N, n = M, lda = N, ldb = K, ldc = N
    sgemmColMajor(N, M, K, 1.0f, B.data(), N, A.data(), K, 0.0f, C.data(), N);
    int bad = 0;
    for (int r = 0; r < M; ++r) {
        for (int c = 0; c < N; ++c) {
            float ref = 0.0f;
            for (int p = 0; p < K; ++p) {
                ref += A[r * K + p] * B[p * N + c];
            }
            std::printf("%5.1f", static_cast<double>(C[r * N + c]));
            bad += (C[r * N + c] != ref);
        }
        std::printf("\n");
    }
    std::printf("mismatches against the row-major definition: %d\n", bad);
    return bad;
}
