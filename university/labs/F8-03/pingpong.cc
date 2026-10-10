// F8-03 Listing 2: measure alpha and beta between two MPI processes with a ping-pong.
// Rank 0 sends m bytes to rank 1, which sends them back; one-way time = round trip / 2.
// For every size: 5 warm-up round trips, then the median of 20 timed ones.
// Fit 1 (two points): alpha = time of the smallest message; beta from the two largest sizes.
// Fit 2: least squares of time against size over all sizes.
// Built with mpic++ and run by run.sh with 2 processes; it prints "fit:" for allreduce_bench.
#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <vector>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    std::vector<long> sizes;
    for (long m = 8; m <= 8L * 1024 * 1024; m *= 4) {
        sizes.push_back(m);
    }
    std::vector<char> buf(static_cast<std::size_t>(sizes.back()), 'x');
    std::vector<double> us;                                     // median one-way time per size
    for (long m : sizes) {
        std::vector<double> t;
        for (int rep = 0; rep < 25; ++rep) {
            MPI_Barrier(MPI_COMM_WORLD);
            const double t0 = MPI_Wtime();
            if (rank == 0) {
                MPI_Send(buf.data(), static_cast<int>(m), MPI_CHAR, 1, 0, MPI_COMM_WORLD);
                MPI_Recv(buf.data(), static_cast<int>(m), MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            } else if (rank == 1) {
                MPI_Recv(buf.data(), static_cast<int>(m), MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Send(buf.data(), static_cast<int>(m), MPI_CHAR, 0, 0, MPI_COMM_WORLD);
            }
            if (rep >= 5) {                                     // skip the warm-up round trips
                t.push_back((MPI_Wtime() - t0) * 1e6 / 2.0);
            }
        }
        std::sort(t.begin(), t.end());
        us.push_back(t[t.size() / 2]);
    }
    if (rank == 0) {
        std::printf("%10s %14s %14s\n", "bytes", "one-way us", "MB/s");
        for (std::size_t i = 0; i < sizes.size(); ++i) {
            std::printf("%10ld %14.2f %14.1f\n", sizes[i], us[i], static_cast<double>(sizes[i]) / us[i]);
        }
        const std::size_t k = sizes.size();
        const double alpha1 = us[0];
        const double beta1 = static_cast<double>(sizes[k - 1] - sizes[k - 2]) / (us[k - 1] - us[k - 2]) / 1e3;
        double sx = 0, sy = 0, sxx = 0, sxy = 0;                // least squares t = a + b * m
        for (std::size_t i = 0; i < k; ++i) {
            const double x = static_cast<double>(sizes[i]);
            sx += x;
            sy += us[i];
            sxx += x * x;
            sxy += x * us[i];
        }
        const double b = (k * sxy - sx * sy) / (k * sxx - sx * sx);
        const double a = (sy - b * sx) / k;
        std::printf("fit 1 (two points): alpha = %.2f us, beta = %.3f GB/s\n", alpha1, beta1);
        std::printf("fit 2 (least squares): alpha = %.2f us, beta = %.3f GB/s\n", a, 1.0 / b / 1e3);
        std::printf("fit: %.4f %.5f\n", alpha1, beta1);         // line read by run.sh
    }
    MPI_Finalize();
    return 0;
}
