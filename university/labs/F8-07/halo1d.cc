// F8-07 Listing 2: a 1-D heat-diffusion stencil with halo exchange (curriculum milestone F4,
// host buffers). The global grid of N cells is split into contiguous blocks, one per rank.
// Each step every rank swaps one boundary cell with each neighbour, then updates its cells.
// The result must be bit-identical for 1, 2 and 4 ranks: every cell is computed from the
// same three operands in the same order, whichever rank owns it.
#include <mpi.h>

#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

namespace {

constexpr int kCells = 1000;   // global grid size
constexpr int kSteps = 500;    // time steps
constexpr double kR = 0.25;    // diffusion number (stable for r <= 0.5)

void check(int rc, const char* what)
{
    if (rc != MPI_SUCCESS) {
        std::fprintf(stderr, "%s failed with code %d\n", what, rc);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

// FNV-1a over the bytes of the doubles: equal hashes mean bit-identical grids.
std::uint64_t fnv1a(const std::vector<double>& v)
{
    std::uint64_t h = 14695981039346656037ULL;
    for (double d : v) {
        unsigned char b[sizeof(double)];
        std::memcpy(b, &d, sizeof(double));
        for (unsigned char c : b) {
            h ^= c;
            h *= 1099511628211ULL;
        }
    }
    return h;
}

}  // namespace

int main(int argc, char** argv)
{
    check(MPI_Init(&argc, &argv), "MPI_Init");
    int rank = 0;
    int size = 0;
    check(MPI_Comm_rank(MPI_COMM_WORLD, &rank), "MPI_Comm_rank");
    check(MPI_Comm_size(MPI_COMM_WORLD, &size), "MPI_Comm_size");

    // Block decomposition: the first (kCells % size) ranks get one extra cell.
    const int base = kCells / size;
    const int extra = kCells % size;
    const int local = base + (rank < extra ? 1 : 0);
    const int first = rank * base + (rank < extra ? rank : extra);

    // u[0] and u[local + 1] are halo cells; u[1..local] are owned cells.
    std::vector<double> u(local + 2, 0.0);
    std::vector<double> next(local + 2, 0.0);
    for (int i = 1; i <= local; ++i) {
        const int g = first + i - 1;               // global index of this cell
        u[i] = (g >= 400 && g < 600) ? 100.0 : 0.0; // a hot band in the middle
    }

    // Edges of the global grid have no neighbour: MPI_PROC_NULL makes the call a no-op.
    const int left = (rank == 0) ? MPI_PROC_NULL : rank - 1;
    const int right = (rank == size - 1) ? MPI_PROC_NULL : rank + 1;

    for (int step = 0; step < kSteps; ++step) {
        // Send my first owned cell left, receive my right halo from the right; then mirror.
        check(MPI_Sendrecv(&u[1], 1, MPI_DOUBLE, left, 0, &u[local + 1], 1, MPI_DOUBLE, right, 0,
                           MPI_COMM_WORLD, MPI_STATUS_IGNORE), "MPI_Sendrecv left");
        check(MPI_Sendrecv(&u[local], 1, MPI_DOUBLE, right, 1, &u[0], 1, MPI_DOUBLE, left, 1,
                           MPI_COMM_WORLD, MPI_STATUS_IGNORE), "MPI_Sendrecv right");
        if (rank == 0) {
            u[0] = 0.0;           // fixed cold boundary on the left
        }
        if (rank == size - 1) {
            u[local + 1] = 0.0;   // fixed cold boundary on the right
        }
        for (int i = 1; i <= local; ++i) {
            next[i] = u[i] + kR * (u[i - 1] - 2.0 * u[i] + u[i + 1]);
        }
        u.swap(next);
    }

    // Gather the owned cells on rank 0 in global order.
    std::vector<int> counts(size);
    std::vector<int> displs(size);
    for (int r = 0; r < size; ++r) {
        counts[r] = base + (r < extra ? 1 : 0);
        displs[r] = r * base + (r < extra ? r : extra);
    }
    std::vector<double> global(rank == 0 ? kCells : 0);
    check(MPI_Gatherv(&u[1], local, MPI_DOUBLE, global.data(), counts.data(), displs.data(),
                      MPI_DOUBLE, 0, MPI_COMM_WORLD), "MPI_Gatherv");
    if (rank == 0) {
        double sum = 0.0;
        for (double d : global) {
            sum += d;   // same order for every rank count
        }
        std::printf("ranks %d: cells %d, steps %d, u[400] = %.12f, u[500] = %.12f, sum = %.12f, fnv1a = %016llx\n",
                    size, kCells, kSteps, global[400], global[500], sum,
                    static_cast<unsigned long long>(fnv1a(global)));
    }
    check(MPI_Finalize(), "MPI_Finalize");
    return 0;
}
