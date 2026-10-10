// F8-03 Listing 3: predict, then measure. Times MPI_Allreduce (float sum) for sizes from
// 8 bytes to 8 MiB, prints algorithm and bus bandwidth, and compares every median with the
// ring formula 2(N-1) * (alpha + (S/N) / beta), using the alpha and beta that Listing 2
// measured (passed as arguments by run.sh). run.sh forces Open MPI's ring algorithm, so
// the formula and the algorithm match. Reduction arithmetic is not in the model.
#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int n = 1;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &n);
    const double alphaUs = (argc > 2) ? std::atof(argv[1]) : 1.0;
    const double betaGBps = (argc > 2) ? std::atof(argv[2]) : 1.0;

    const long maxBytes = 8L * 1024 * 1024;
    std::vector<float> in(static_cast<std::size_t>(maxBytes / 4), 1.0f), out(in.size());
    if (rank == 0) {
        std::printf("ranks %d; model: alpha = %.2f us, beta = %.3f GB/s (from the ping-pong)\n", n, alphaUs, betaGBps);
        std::printf("%10s %13s %13s %10s %10s %11s\n", "bytes", "measured us", "predicted us", "algbw GB/s",
                    "busbw GB/s", "meas/pred");
    }
    for (long bytes = 8; bytes <= maxBytes; bytes *= 4) {
        const int count = static_cast<int>(bytes / 4);
        std::vector<double> t;
        for (int rep = 0; rep < 25; ++rep) {
            MPI_Barrier(MPI_COMM_WORLD);
            const double t0 = MPI_Wtime();
            MPI_Allreduce(in.data(), out.data(), count, MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
            double local = (MPI_Wtime() - t0) * 1e6;
            double slowest = 0.0;                         // a collective ends when the last rank ends
            MPI_Allreduce(&local, &slowest, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            if (rep >= 5) {
                t.push_back(slowest);
            }
        }
        std::sort(t.begin(), t.end());
        const double us = t[t.size() / 2];
        const double s = static_cast<double>(bytes);
        const double predicted = 2.0 * (n - 1) * (alphaUs + (s / n) / (betaGBps * 1e3));
        const double algbw = s / us / 1e3;
        const double busbw = algbw * 2.0 * (n - 1) / n;
        if (rank == 0) {
            std::printf("%10ld %13.2f %13.2f %10.3f %10.3f %11.2f\n", bytes, us, predicted, algbw, busbw, us / predicted);
        }
    }
    bool ok = true;                                       // the last result must be the sum n * 1.0
    for (float v : out) {
        ok = ok && (v == static_cast<float>(n));
    }
    int bad = ok ? 0 : 1;
    int anyBad = 0;
    MPI_Allreduce(&bad, &anyBad, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    if (rank == 0) {
        std::printf("result check (every element equals %d): %s\n", n, anyBad ? "FAILED" : "ok");
    }
    MPI_Finalize();
    return anyBad;
}
