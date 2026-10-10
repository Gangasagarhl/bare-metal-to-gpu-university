// BR-04 Listing 4: predict, then run. Rank 0 first prints the alpha-beta prediction of a
// ring all-reduce for every size, using the alpha and beta measured by Listing 3 (passed as
// arguments). Only then does the program run its own ring all-reduce (reduce-scatter, then
// all-gather, with MPI_Sendrecv to the neighbours), time it (5 warm-up runs, median of 20;
// one run = the slowest rank's time) and check every result against MPI_Allreduce.
// Usage (run.sh): mpirun -np 4 ./ring_allreduce <alpha_us> <beta_GBps> <label>
#include <mpi.h>
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <vector>

namespace {

// Ring all-reduce (sum) of data, in place. data.size() must be a multiple of the rank count.
void ringAllReduce(std::vector<float>& data, std::vector<float>& tmp, int me, int n)
{
    const int seg = static_cast<int>(data.size()) / n;
    const int right = (me + 1) % n;
    const int left = (me + n - 1) % n;
    // Reduce-scatter: after n-1 steps, rank me holds the full sum of segment (me + 1) % n.
    for (int s = 0; s < n - 1; ++s) {
        const int sendSeg = (me - s + n) % n;
        const int recvSeg = (me - s - 1 + 2 * n) % n;
        MPI_Sendrecv(data.data() + sendSeg * seg, seg, MPI_FLOAT, right, s,
                     tmp.data(), seg, MPI_FLOAT, left, s, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        for (int i = 0; i < seg; ++i) {
            data[recvSeg * seg + i] += tmp[i];
        }
    }
    // All-gather: pass the finished segments around the ring, n-1 more steps.
    for (int s = 0; s < n - 1; ++s) {
        const int sendSeg = (me + 1 - s + n) % n;
        const int recvSeg = (me - s + n) % n;
        MPI_Sendrecv(data.data() + sendSeg * seg, seg, MPI_FLOAT, right, n + s,
                     data.data() + recvSeg * seg, seg, MPI_FLOAT, left, n + s, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
}

// The alpha-beta model of the ring: 2(n-1) steps, each moving bytes/n.
double predictUs(double alphaUs, double betaGBps, int n, double bytes)
{
    return 2.0 * (n - 1) * (alphaUs + bytes / n / (betaGBps * 1e3));
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int me = 0;
    int n = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &n);
    if (argc < 4) {
        if (me == 0) {
            std::printf("usage: ring_allreduce <alpha_us> <beta_GBps> <label>\n");
        }
        MPI_Finalize();
        return 2;
    }
    const double alpha = std::atof(argv[1]);
    const double beta = std::atof(argv[2]);
    std::vector<long> counts;                                   // floats per rank, multiples of n
    for (long c = n; c <= 4L * 1024 * 1024; c *= 4) {
        counts.push_back(c);
    }

    // 1. The prediction, printed before anything is measured.
    if (me == 0) {
        std::printf("PREDICTION (%s): alpha = %.2f us, beta = %.3f GB/s, %d ranks, ring = 2(N-1) steps\n",
                    argv[3], alpha, beta, n);
        std::printf("%10s %14s %14s %14s\n", "bytes", "alpha part us", "beta part us", "predicted us");
        for (long c : counts) {
            const double bytes = 4.0 * c;
            const double t = predictUs(alpha, beta, n, bytes);
            std::printf("%10.0f %14.2f %14.2f %14.2f\n", bytes, 2.0 * (n - 1) * alpha, t - 2.0 * (n - 1) * alpha, t);
        }
        std::fflush(stdout);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // 2. The run.
    if (me == 0) {
        std::printf("MEASURED (own ring, median of 20, slowest rank)\n");
        std::printf("%10s %12s %12s %10s %10s %9s\n", "bytes", "measured us", "predicted us", "algbw GB/s",
                    "busbw GB/s", "meas/pred");
    }
    long wrong = 0;
    for (long c : counts) {
        std::vector<float> data(static_cast<std::size_t>(c));
        std::vector<float> tmp(static_cast<std::size_t>(c / n));
        std::vector<float> ref(static_cast<std::size_t>(c));
        std::vector<double> t;
        for (int r = -5; r < 20; ++r) {
            for (long i = 0; i < c; ++i) {
                data[static_cast<std::size_t>(i)] = static_cast<float>(me + 1 + i % 7);  // small integers: exact sums
            }
            MPI_Barrier(MPI_COMM_WORLD);
            const double t0 = MPI_Wtime();
            ringAllReduce(data, tmp, me, n);
            double dt = (MPI_Wtime() - t0) * 1e6;
            MPI_Allreduce(MPI_IN_PLACE, &dt, 1, MPI_DOUBLE, MPI_MAX, MPI_COMM_WORLD);
            if (r >= 0) {
                t.push_back(dt);
            }
        }
        for (long i = 0; i < c; ++i) {
            ref[static_cast<std::size_t>(i)] = static_cast<float>(me + 1 + i % 7);
        }
        MPI_Allreduce(MPI_IN_PLACE, ref.data(), static_cast<int>(c), MPI_FLOAT, MPI_SUM, MPI_COMM_WORLD);
        for (long i = 0; i < c; ++i) {
            wrong += data[static_cast<std::size_t>(i)] != ref[static_cast<std::size_t>(i)];
        }
        std::sort(t.begin(), t.end());
        const double med = (t[9] + t[10]) / 2.0;
        if (me == 0) {
            const double bytes = 4.0 * c;
            const double pred = predictUs(alpha, beta, n, bytes);
            const double algbw = bytes / med / 1e3;
            std::printf("%10.0f %12.2f %12.2f %10.3f %10.3f %9.2f\n", bytes, med, pred, algbw,
                        algbw * 2.0 * (n - 1) / n, med / pred);
        }
    }
    MPI_Allreduce(MPI_IN_PLACE, &wrong, 1, MPI_LONG, MPI_SUM, MPI_COMM_WORLD);
    if (me == 0) {
        std::printf("elements that differ from MPI_Allreduce, all sizes, all ranks: %ld\n", wrong);
    }
    MPI_Finalize();
    return wrong == 0 ? 0 : 1;
}
