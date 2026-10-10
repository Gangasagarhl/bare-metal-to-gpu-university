// F8-13 Listing 1: tensor parallelism for one MLP block, Z = tanh(X A) B, with MPI.
// A (h x f) is split by columns and B (f x h) by rows over the N ranks (column-parallel, then
// row-parallel), so each rank computes its slice of the hidden layer without communication
// and one all-reduce adds the partial outputs. The backward pass needs one all-reduce for dX.
// Every rank also computes the whole block alone (the reference) and compares.
// Arguments: ffn=<f> (default 32); check=off skips the divisibility check (forensic lab).
#include <mpi.h>

#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <vector>

constexpr int kB = 4;    // rows of X (tokens in the batch)
constexpr int kH = 8;    // model width h

using Mat = std::vector<float>;     // row-major

std::uint64_t mix(std::uint64_t z)
{
    z += 0x9E3779B97F4A7C15ull;
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ull;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBull;
    return z ^ (z >> 31);
}

Mat randomMat(int rows, int cols, std::uint64_t seed)
{
    Mat m(rows * cols);
    for (int i = 0; i < rows * cols; ++i) {
        m[i] = static_cast<float>(mix((seed << 32) + i) >> 40) / 8388608.0f - 1.0f;
    }
    return m;
}

// C (n x m) = A (n x k) * B (k x m); A, B may be transposed views through the strides
Mat matmul(const Mat& a, int n, int k, const Mat& b, int m, bool transA = false, bool transB = false)
{
    Mat c(n * m, 0.0f);
    for (int i = 0; i < n; ++i) {
        for (int j = 0; j < m; ++j) {
            float s = 0.0f;
            for (int t = 0; t < k; ++t) {
                const float x = transA ? a[t * n + i] : a[i * k + t];
                const float y = transB ? b[j * k + t] : b[t * m + j];
                s += x * y;
            }
            c[i * m + j] = s;
        }
    }
    return c;
}

Mat columns(const Mat& m, int rows, int cols, int first, int count)   // m[:, first:first+count]
{
    Mat out(rows * count);
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < count; ++j) out[i * count + j] = m[i * cols + first + j];
    }
    return out;
}

Mat rowsOf(const Mat& m, int cols, int first, int count)            // m[first:first+count, :]
{
    return Mat(m.begin() + first * cols, m.begin() + (first + count) * cols);
}

float maxDiff(const Mat& a, const Mat& b)
{
    float d = 0.0f;
    for (std::size_t i = 0; i < a.size(); ++i) {
        const float e = std::fabs(a[i] - b[i]);
        if (!(e <= d)) d = e;
    }
    return d;
}

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0, size = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int f = 32;
    bool check = true;
    for (int i = 1; i < argc; ++i) {
        const std::string a = argv[i];
        if (a.rfind("ffn=", 0) == 0) f = std::atoi(a.c_str() + 4);
        if (a == "check=off") check = false;
    }
    if (check && (f % size != 0 || kB % size != 0)) {
        if (rank == 0) std::printf("refusing: ffn %d or batch %d is not divisible by %d ranks\n", f, kB, size);
        MPI_Finalize();
        return 2;
    }
    const int fr = f / size;              // hidden columns owned by this rank
    const int c0 = rank * fr;

    const Mat X = randomMat(kB, kH, 1), A = randomMat(kH, f, 2), B = randomMat(f, kH, 3);
    const Mat T = randomMat(kB, kH, 4);   // target, for the loss 0.5 * |Z - T|^2

    // reference: the whole block on one process
    Mat U = matmul(X, kB, kH, A, f), H(U.size());
    for (std::size_t i = 0; i < U.size(); ++i) H[i] = std::tanh(U[i]);
    const Mat Zref = matmul(H, kB, f, B, kH);
    Mat dZref(Zref.size());
    for (std::size_t i = 0; i < Zref.size(); ++i) dZref[i] = Zref[i] - T[i];
    Mat dH = matmul(dZref, kB, kH, B, f, false, true);               // dZ B^T
    for (std::size_t i = 0; i < dH.size(); ++i) dH[i] *= 1.0f - H[i] * H[i];
    const Mat dXref = matmul(dH, kB, f, A, kH, false, true);         // dU A^T
    const Mat dAref = matmul(X, kH, kB, dH, f, true, false);         // X^T dU

    // tensor-parallel forward: no communication until the partial outputs are added
    const Mat Ar = columns(A, kH, f, c0, fr), Br = rowsOf(B, kH, c0, fr);
    Mat Hr = matmul(X, kB, kH, Ar, fr);
    for (float& v : Hr) v = std::tanh(v);
    Mat Z = matmul(Hr, kB, fr, Br, kH);                              // partial sum of Z
    const Mat Zpartial = Z;
    MPI_Allreduce(MPI_IN_PLACE, Z.data(), kB * kH, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);   // g
    long sent = kB * kH;

    // backward: dZ is the same on every rank; dA and dB slices need no communication
    Mat dZ(Z.size());
    for (std::size_t i = 0; i < Z.size(); ++i) dZ[i] = Z[i] - T[i];
    Mat dHr = matmul(dZ, kB, kH, Br, fr, false, true);
    for (std::size_t i = 0; i < dHr.size(); ++i) dHr[i] *= 1.0f - Hr[i] * Hr[i];
    const Mat dAr = matmul(X, kH, kB, dHr, fr, true, false);
    Mat dX = matmul(dHr, kB, fr, Ar, kH, false, true);               // partial sum of dX
    MPI_Allreduce(MPI_IN_PLACE, dX.data(), kB * kH, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD); // f backward
    sent += kB * kH;

    // the same output with reduce-scatter (each rank gets kB/N rows) followed by all-gather
    Mat Zrs(kB * kH);
    if (kB % size == 0) {
        Mat mine(kB / size * kH);
        MPI_Reduce_scatter_block(Zpartial.data(), mine.data(), kB / size * kH, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        MPI_Allgather(mine.data(), kB / size * kH, MPI_FLOAT, Zrs.data(), kB / size * kH, MPI_FLOAT, MPI_COMM_WORLD);
    }

    // a wrong split for comparison: A by rows, tanh applied to partial sums before adding them
    const int hr = kH / size;
    Mat Uwrong = matmul(columns(X, kB, kH, rank * hr, hr), kB, hr, rowsOf(A, f, rank * hr, hr), f);
    for (float& v : Uwrong) v = std::tanh(v);
    MPI_Allreduce(MPI_IN_PLACE, Uwrong.data(), kB * f, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);

    float dA = maxDiff(dAr, columns(dAref, kH, f, c0, fr));
    MPI_Allreduce(MPI_IN_PLACE, &dA, 1, MPI_FLOAT, MPI_MAX, MPI_COMM_WORLD);
    if (rank == 0) {
        std::printf("N = %d ranks, X %dx%d, A %dx%d split into %d columns per rank, B split into %d rows per rank\n",
                    size, kB, kH, kH, f, fr, fr);
        std::printf("max |Z - Z_ref|          (all-reduce)              = %.3g\n", maxDiff(Z, Zref));
        std::printf("max |Z - Z_ref|          (reduce-scatter+all-gather) = %.3g\n", maxDiff(Zrs, Zref));
        std::printf("max |dX - dX_ref|        (all-reduce in backward)  = %.3g\n", maxDiff(dX, dXref));
        std::printf("max |dA slice - ref|     (no communication)        = %.3g\n", dA);
        std::printf("max |tanh(sum) - sum(tanh)| (wrong split of A)     = %.3g\n", maxDiff(Uwrong, H));
        std::printf("floats all-reduced per rank per step: %ld (forward %d + backward %d); "
                    "the wrong split would need %d before tanh\n", sent, kB * kH, kB * kH, kB * f);
        const bool ok = maxDiff(Z, Zref) <= 1e-5f && maxDiff(Zrs, Zref) <= 1e-5f &&
                        maxDiff(dX, dXref) <= 1e-5f && dA <= 1e-5f;
        std::printf("%s (tolerance 1e-5)\n", ok ? "MATCH" : "DIFFERENT");
    }
    MPI_Finalize();
    return 0;
}
