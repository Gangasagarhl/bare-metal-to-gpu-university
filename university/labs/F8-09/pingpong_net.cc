// F8-09 Listing 1: one-way time and bandwidth between ranks 0 and 1 with host buffers,
// plus an alpha-beta fit. Run it once per transport (the run script forces the transport
// with MCA options) to compare the paths a message can take.
#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <vector>

namespace {

double oneWay(int rank, std::vector<char>& buf, int bytes, int iters)
{
    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    for (int i = 0; i < iters; ++i) {
        if (rank == 0) {
            MPI_Send(buf.data(), bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(buf.data(), bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else if (rank == 1) {
            MPI_Recv(buf.data(), bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Send(buf.data(), bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
    }
    return (MPI_Wtime() - t0) / (2.0 * iters);
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const int maxBytes = 8 << 20;
    std::vector<char> buf(maxBytes, 1);
    std::vector<double> sizes;
    std::vector<double> times;
    if (rank == 0) {
        std::printf("%10s %12s %12s\n", "bytes", "one-way us", "MB/s");
    }
    for (int bytes = 8; bytes <= maxBytes; bytes *= 4) {
        const int iters = bytes <= 65536 ? 1000 : 50;
        oneWay(rank, buf, bytes, 10);   // warm-up
        std::vector<double> t(9);
        for (double& x : t) {
            x = oneWay(rank, buf, bytes, iters);
        }
        std::sort(t.begin(), t.end());   // median of 9 repetitions
        if (rank == 0) {
            std::printf("%10d %12.2f %12.1f\n", bytes, t[4] * 1e6, bytes / t[4] / 1e6);
        }
        sizes.push_back(bytes);
        times.push_back(t[4]);
    }
    if (rank == 0) {
        // Two-point fit: alpha from the smallest message, beta from the two largest.
        const std::size_t n = sizes.size();
        const double beta = (sizes[n - 1] - sizes[n - 2]) / (times[n - 1] - times[n - 2]);
        std::printf("fit: alpha = %.2f us (8-byte one-way time), beta = %.2f GB/s (two largest sizes)\n",
                    times[0] * 1e6, beta / 1e9);
    }
    MPI_Finalize();
    return 0;
}
