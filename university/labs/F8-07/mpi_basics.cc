// F8-07 Listing 1: MPI point-to-point and collectives, checked against values we can
// compute by hand. Every rank checks; only rank 0 prints, so the output is deterministic.
#include <mpi.h>

#include <cstdio>
#include <numeric>
#include <vector>

namespace {

void check(int rc, const char* what)
{
    if (rc != MPI_SUCCESS) {
        std::fprintf(stderr, "%s failed with code %d\n", what, rc);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }
}

// Every rank reports pass (1) or fail (0); rank 0 prints the minimum over all ranks.
void report(const char* name, bool ok, int rank)
{
    int mine = ok ? 1 : 0;
    int all = 0;
    check(MPI_Reduce(&mine, &all, 1, MPI_INT, MPI_MIN, 0, MPI_COMM_WORLD), "MPI_Reduce");
    if (rank == 0) {
        std::printf("  %-28s %s\n", name, all == 1 ? "PASS" : "FAIL");
    }
}

}  // namespace

int main(int argc, char** argv)
{
    check(MPI_Init(&argc, &argv), "MPI_Init");
    int rank = 0;
    int size = 0;
    check(MPI_Comm_rank(MPI_COMM_WORLD, &rank), "MPI_Comm_rank");
    check(MPI_Comm_size(MPI_COMM_WORLD, &size), "MPI_Comm_size");
    if (rank == 0) {
        std::printf("ranks: %d\n", size);
    }

    // 1. Blocking ring with MPI_Sendrecv: pass my rank to the right, receive from the left.
    const int right = (rank + 1) % size;
    const int left = (rank - 1 + size) % size;
    int fromLeft = -1;
    check(MPI_Sendrecv(&rank, 1, MPI_INT, right, 7, &fromLeft, 1, MPI_INT, left, 7,
                       MPI_COMM_WORLD, MPI_STATUS_IGNORE), "MPI_Sendrecv");
    report("Sendrecv ring", fromLeft == left, rank);

    // 2. Non-blocking exchange with both neighbours: post receives first, then sends, wait all.
    std::vector<double> sendBuf(4, 100.0 * rank);
    std::vector<double> recvL(4, -1.0);
    std::vector<double> recvR(4, -1.0);
    MPI_Request req[4];
    check(MPI_Irecv(recvL.data(), 4, MPI_DOUBLE, left, 1, MPI_COMM_WORLD, &req[0]), "MPI_Irecv");
    check(MPI_Irecv(recvR.data(), 4, MPI_DOUBLE, right, 2, MPI_COMM_WORLD, &req[1]), "MPI_Irecv");
    check(MPI_Isend(sendBuf.data(), 4, MPI_DOUBLE, right, 1, MPI_COMM_WORLD, &req[2]), "MPI_Isend");
    check(MPI_Isend(sendBuf.data(), 4, MPI_DOUBLE, left, 2, MPI_COMM_WORLD, &req[3]), "MPI_Isend");
    check(MPI_Waitall(4, req, MPI_STATUSES_IGNORE), "MPI_Waitall");
    report("Isend/Irecv neighbours", recvL[3] == 100.0 * left && recvR[0] == 100.0 * right, rank);

    // 3. Broadcast from rank 0.
    int value = (rank == 0) ? 42 : 0;
    check(MPI_Bcast(&value, 1, MPI_INT, 0, MPI_COMM_WORLD), "MPI_Bcast");
    report("Bcast", value == 42, rank);

    // 4. Allreduce: every rank contributes rank + 1; the sum is size * (size + 1) / 2.
    int contrib = rank + 1;
    int total = 0;
    check(MPI_Allreduce(&contrib, &total, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD), "MPI_Allreduce");
    report("Allreduce (sum of rank+1)", total == size * (size + 1) / 2, rank);

    // 5. Allgather: every rank ends with [0, 1, ..., size-1].
    std::vector<int> gathered(size, -1);
    check(MPI_Allgather(&rank, 1, MPI_INT, gathered.data(), 1, MPI_INT, MPI_COMM_WORLD),
          "MPI_Allgather");
    std::vector<int> expect(size);
    std::iota(expect.begin(), expect.end(), 0);
    report("Allgather", gathered == expect, rank);

    // 6. Reduce_scatter_block: vector of length size, element j = rank * 10 + j.
    //    Rank j receives sum over r of (r * 10 + j) = 10 * size * (size - 1) / 2 + size * j.
    std::vector<int> full(size);
    for (int j = 0; j < size; ++j) {
        full[j] = rank * 10 + j;
    }
    int myBlock = -1;
    check(MPI_Reduce_scatter_block(full.data(), &myBlock, 1, MPI_INT, MPI_SUM, MPI_COMM_WORLD),
          "MPI_Reduce_scatter_block");
    report("Reduce_scatter_block", myBlock == 10 * size * (size - 1) / 2 + size * rank, rank);

    // 7. Alltoall: rank r sends value r * 100 + d to rank d; rank d receives s * 100 + d from s.
    std::vector<int> out(size);
    std::vector<int> in(size, -1);
    for (int d = 0; d < size; ++d) {
        out[d] = rank * 100 + d;
    }
    check(MPI_Alltoall(out.data(), 1, MPI_INT, in.data(), 1, MPI_INT, MPI_COMM_WORLD),
          "MPI_Alltoall");
    bool okA = true;
    for (int s = 0; s < size; ++s) {
        okA = okA && in[s] == s * 100 + rank;
    }
    report("Alltoall", okA, rank);

    if (rank == 0) {
        std::printf("Allreduce result on every rank: %d (expected %d)\n", total,
                    size * (size + 1) / 2);
    }
    check(MPI_Finalize(), "MPI_Finalize");
    return 0;
}
