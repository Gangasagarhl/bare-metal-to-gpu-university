// F6-35 Listing 1: the BLAS calling convention, modelled on the CPU.
// sgemmColMajor() follows the column-major contract of the BLAS SGEMM routine that cuBLAS
// implements: C = alpha * op(A) * op(B) + beta * C, where element (i, j) of a column-major
// matrix X with leading dimension ldX is X[i + j * ldX]. Our matrices are row-major C++ arrays.
#include <cstdio>
#include <iostream>
#include <string>
#include <vector>

void sgemmColMajor(bool transA, bool transB, int m, int n, int k, float alpha, const float* a, int lda,
                   const float* b, int ldb, float beta, float* c, int ldc)
{
    for (int j = 0; j < n; ++j) {
        for (int i = 0; i < m; ++i) {
            float acc = 0.0f;
            for (int p = 0; p < k; ++p) {
                const float aip = transA ? a[p + i * lda] : a[i + p * lda];
                const float bpj = transB ? b[j + p * ldb] : b[p + j * ldb];
                acc += aip * bpj;
            }
            c[i + j * ldc] = alpha * acc + beta * c[i + j * ldc];
        }
    }
}

void printRowMajor(const char* title, const std::vector<float>& x, int rows, int cols, int ld)
{
    std::printf("%s\n", title);
    for (int r = 0; r < rows; ++r) {
        std::printf("   ");
        for (int c = 0; c < cols; ++c) {
            std::printf(" %6.0f", static_cast<double>(x[static_cast<std::size_t>(r * ld + c)]));
        }
        std::printf("\n");
    }
}

int main()
{
    // A is 2 x 3 and B is 3 x 4, stored row-major (C++ style). We want C = A * B, 2 x 4, row-major.
    const int m = 2;
    const int k = 3;
    const int n = 4;
    const std::vector<float> a = {1, 2, 3,
                                  4, 5, 6};
    const std::vector<float> b = {1, 0, 2, 1,
                                  0, 1, 1, 2,
                                  1, 1, 0, 3};
    std::vector<float> ref(static_cast<std::size_t>(m * n), 0.0f);
    for (int i = 0; i < m; ++i) {
        for (int j = 0; j < n; ++j) {
            for (int p = 0; p < k; ++p) {
                ref[static_cast<std::size_t>(i * n + j)] += a[static_cast<std::size_t>(i * k + p)] *
                                                             b[static_cast<std::size_t>(p * n + j)];
            }
        }
    }
    printRowMajor("reference C = A * B (plain loops, row-major):", ref, m, n, n);

    std::string mode;
    while (std::cin >> mode) {
        std::vector<float> c(static_cast<std::size_t>(m * n), 0.0f);
        if (mode == "as_if_colmajor") {
            // mistake: the textbook column-major call (lda = m, ldb = k, ldc = m) on row-major arrays
            sgemmColMajor(false, false, m, n, k, 1.0f, a.data(), m, b.data(), k, 0.0f, c.data(), m);
        } else if (mode == "swap_operands") {
            // C^T = B^T * A^T: a row-major matrix read as column-major IS its transpose,
            // so ask for an n x m product of B and A and get row-major C for free
            sgemmColMajor(false, false, n, m, k, 1.0f, b.data(), n, a.data(), k, 0.0f, c.data(), n);
        } else {
            std::printf("unknown mode %s\n", mode.c_str());
            return 1;
        }
        int wrong = 0;
        for (std::size_t i = 0; i < c.size(); ++i) {
            wrong += (c[i] != ref[i]) ? 1 : 0;
        }
        const std::string title = "mode " + mode + " (C printed row-major):";
        printRowMajor(title.c_str(), c, m, n, n);
        std::printf("    %d of %d elements differ from the reference\n", wrong, m * n);
    }
    return 0;
}
