// F8-07 Listing 3 (forensic lab): two ranks that both call MPI_Send first, then MPI_Recv
// ("head to head"). Whether this completes depends on the message size and on the MPI
// library's buffering, not on the MPI Standard. Mode "fixed" uses MPI_Sendrecv instead.
#include <mpi.h>

#include <cstdio>
#include <cstring>
#include <vector>

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    const bool fixed = (argc > 1 && std::strcmp(argv[1], "fixed") == 0);
    const int peer = 1 - rank;   // run with exactly 2 ranks

    for (int bytes = 1024; bytes <= 65536; bytes *= 2) {
        std::vector<char> out(bytes, static_cast<char>('a' + rank));
        std::vector<char> in(bytes, 0);
        if (rank == 0) {
            std::printf("%-6s %6d bytes: entering exchange\n", fixed ? "fixed" : "naive", bytes);
            std::fflush(stdout);
        }
        if (fixed) {
            MPI_Sendrecv(out.data(), bytes, MPI_CHAR, peer, 0, in.data(), bytes, MPI_CHAR, peer, 0,
                         MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        } else {
            MPI_Send(out.data(), bytes, MPI_CHAR, peer, 0, MPI_COMM_WORLD);
            MPI_Recv(in.data(), bytes, MPI_CHAR, peer, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
        if (rank == 0) {
            std::printf("%-6s %6d bytes: done, received '%c'\n", fixed ? "fixed" : "naive", bytes,
                        in[0]);
            std::fflush(stdout);
        }
    }
    MPI_Finalize();
    return 0;
}
