// F8-08 Listing 2: a ping-pong latency and bandwidth benchmark between ranks 0 and 1,
// in two modes:
//   direct - MPI sends from and receives into the application buffer;
//   staged - before each send the data is copied from the application buffer into a
//            separate "bounce" buffer, and after each receive it is copied back out,
//            which is the extra work a host-staged path adds.
// On this machine both buffers are host memory, so the extra copies are plain memcpy:
// a CPU model of staging, not a GPU measurement.
#include <mpi.h>

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

double pingPong(int rank, std::vector<char>& app, std::vector<char>& bounce, int bytes,
                int iters, bool staged)
{
    char* wire = staged ? bounce.data() : app.data();
    MPI_Barrier(MPI_COMM_WORLD);
    const double t0 = MPI_Wtime();
    for (int i = 0; i < iters; ++i) {
        if (rank == 0) {
            if (staged) {
                std::memcpy(bounce.data(), app.data(), bytes);   // "device to host"
            }
            MPI_Send(wire, bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD);
            MPI_Recv(wire, bytes, MPI_CHAR, 1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (staged) {
                std::memcpy(app.data(), bounce.data(), bytes);   // "host to device"
            }
        } else if (rank == 1) {
            MPI_Recv(wire, bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if (staged) {
                std::memcpy(app.data(), bounce.data(), bytes);
                std::memcpy(bounce.data(), app.data(), bytes);
            }
            MPI_Send(wire, bytes, MPI_CHAR, 0, 0, MPI_COMM_WORLD);
        }
    }
    const double t1 = MPI_Wtime();
    return (t1 - t0) / (2.0 * iters);   // one-way time in seconds
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const int maxBytes = 8 << 20;   // 8 MiB
    std::vector<char> app(maxBytes, 1);
    std::vector<char> bounce(maxBytes, 0);
    if (rank == 0) {
        std::printf("%10s %14s %14s %12s %12s %8s\n", "bytes", "direct us", "staged us",
                    "direct MB/s", "staged MB/s", "ratio");
    }
    for (int bytes = 8; bytes <= maxBytes; bytes *= 4) {
        const int iters = bytes <= 65536 ? 2000 : 100;
        pingPong(rank, app, bounce, bytes, 10, false);   // warm-up, not counted
        // Median of 9 repetitions per mode, to damp noise on a shared machine.
        std::vector<double> d(9);
        std::vector<double> s(9);
        for (int k = 0; k < 9; ++k) {
            d[k] = pingPong(rank, app, bounce, bytes, iters, false);
            s[k] = pingPong(rank, app, bounce, bytes, iters, true);
        }
        std::sort(d.begin(), d.end());
        std::sort(s.begin(), s.end());
        if (rank == 0) {
            std::printf("%10d %14.2f %14.2f %12.1f %12.1f %8.2f\n", bytes, d[4] * 1e6, s[4] * 1e6,
                        bytes / d[4] / 1e6, bytes / s[4] / 1e6, s[4] / d[4]);
        }
    }
    MPI_Finalize();
    return 0;
}
