// BR-04 Listing 3: a peer latency and bandwidth matrix (the shape of milestone F1) for the
// MPI processes of one machine. Every ordered pair (a, b) does a ping-pong: 5 warm-up round
// trips, then the median of 20 timed ones; one-way time = round trip / 2. Two sizes per pair:
// 8 B (latency) and 4 MiB (bandwidth). Then a size sweep on the pair 0 -> 1 gives alpha and
// beta (alpha = time of the smallest message; beta = largest message / (its time - alpha))
// and prints them on a "fit:" line for run.sh.
// Built with mpic++ and run by run.sh twice: shared memory, and TCP over the loopback device.
#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

constexpr int kWarmup = 5;
constexpr int kRuns = 20;

// Median one-way time in microseconds of a bytes-long ping-pong between ranks a and b.
// Every rank calls it (the barrier is a collective); only rank a's result is meaningful.
double oneWayUs(int me, int a, int b, std::vector<char>& buf, int bytes)
{
    std::vector<double> t;
    for (int r = -kWarmup; r < kRuns; ++r) {
        MPI_Barrier(MPI_COMM_WORLD);
        const double t0 = MPI_Wtime();
        if (me == a) {
            MPI_Send(buf.data(), bytes, MPI_CHAR, b, 0, MPI_COMM_WORLD);
            MPI_Recv(buf.data(), bytes, MPI_CHAR, b, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (me == b) {
            MPI_Recv(buf.data(), bytes, MPI_CHAR, a, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(buf.data(), bytes, MPI_CHAR, a, 0, MPI_COMM_WORLD);
        }
        if (r >= 0) {
            t.push_back((MPI_Wtime() - t0) * 1e6 / 2.0);
        }
    }
    std::sort(t.begin(), t.end());
    return (t[kRuns / 2 - 1] + t[kRuns / 2]) / 2.0;
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int me = 0;
    int n = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &n);
    const int small = 8;
    const int big = 4 * 1024 * 1024;
    std::vector<char> buf(16 * 1024 * 1024, 'x');

    // Row a of each matrix is filled by rank a, then everything is collected on rank 0.
    std::vector<double> lat(n * n, 0.0);
    std::vector<double> bw(n * n, 0.0);
    for (int a = 0; a < n; ++a) {
        for (int b = 0; b < n; ++b) {
            if (a == b) {
                continue;
            }
            const double tSmall = oneWayUs(me, a, b, buf, small);
            const double tBig = oneWayUs(me, a, b, buf, big);
            if (me == a) {
                lat[a * n + b] = tSmall;
                bw[a * n + b] = big / tBig / 1e3;              // bytes per us / 1e3 = GB/s
            }
        }
    }
    MPI_Reduce(me == 0 ? MPI_IN_PLACE : lat.data(), lat.data(), n * n, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(me == 0 ? MPI_IN_PLACE : bw.data(), bw.data(), n * n, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

    // Size sweep on the pair 0 -> 1 (every rank takes part in the barriers).
    std::vector<int> sizes;
    for (int m = 8; m <= 16 * 1024 * 1024; m *= 4) {
        sizes.push_back(m);
    }
    std::vector<double> sweep;
    for (int m : sizes) {
        sweep.push_back(oneWayUs(me, 0, 1, buf, m));
    }

    if (me == 0) {
        std::printf("%d processes; one-way latency (us, 8 B messages), row = sender, column = receiver\n", n);
        for (int a = 0; a < n; ++a) {
            std::printf("  %d:", a);
            for (int b = 0; b < n; ++b) {
                a == b ? std::printf("%9s", "-") : std::printf("%9.2f", lat[a * n + b]);
            }
            std::printf("\n");
        }
        std::printf("bandwidth (GB/s, 4 MiB messages, 1 GB = 10^9 bytes)\n");
        for (int a = 0; a < n; ++a) {
            std::printf("  %d:", a);
            for (int b = 0; b < n; ++b) {
                a == b ? std::printf("%9s", "-") : std::printf("%9.3f", bw[a * n + b]);
            }
            std::printf("\n");
        }
        std::printf("size sweep, pair 0 -> 1\n%10s %12s %10s\n", "bytes", "one-way us", "GB/s");
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            std::printf("%10d %12.2f %10.3f\n", sizes[i], sweep[i], sizes[i] / sweep[i] / 1e3);
        }
        const std::size_t k = sizes.size();
        const double alpha = sweep[0];
        const double beta = sizes[k - 1] / (sweep[k - 1] - alpha) / 1e3;
        std::printf("fit from the smallest and the largest size: alpha = %.2f us, beta = %.3f GB/s, half-bandwidth size alpha*beta = %.0f bytes\n",
                    alpha, beta, alpha * beta * 1e3);
        std::printf("fit: %.4f %.5f\n", alpha, beta);
    }
    MPI_Finalize();
    return 0;
}
