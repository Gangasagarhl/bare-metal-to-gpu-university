// BR-04 Listing 5: the third trap, mismatched collective calls. Each rank holds three
// gradient "buckets" and all-reduces them one by one, in the order in which they became
// ready. In "ready" mode that order depends on the rank (as it can when backward passes
// finish layers at different moments); in "fixed" mode every rank uses the same agreed order.
// Usage (run.sh): mpirun -np 4 ./mismatch ready|fixed   (run under a time limit)
#include <mpi.h>
#include <cstdio>
#include <string>
#include <vector>

namespace {

struct Bucket
{
    std::string name;
    std::vector<float> grad;
};

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int me = 0;
    int n = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &me);
    MPI_Comm_size(MPI_COMM_WORLD, &n);
    const bool fixed = argc > 1 && std::string(argv[1]) == "fixed";

    std::vector<Bucket> buckets = {
        {"layer1", std::vector<float>(1000, 1.0f)},
        {"layer2", std::vector<float>(250000, 1.0f)},
        {"layer3", std::vector<float>(1000, 1.0f)},
    };
    std::vector<int> order = {0, 1, 2};                          // the agreed order
    if (!fixed && me % 2 == 1) {
        order = {1, 0, 2};                                       // odd ranks: layer2 was ready first
    }
    for (int k : order) {
        Bucket& b = buckets[static_cast<std::size_t>(k)];
        std::printf("rank %d: all-reduce %s (%zu floats)\n", me, b.name.c_str(), b.grad.size());
        std::fflush(stdout);
        MPI_Allreduce(MPI_IN_PLACE, b.grad.data(), static_cast<int>(b.grad.size()), MPI_FLOAT, MPI_SUM,
                      MPI_COMM_WORLD);
    }
    bool ok = true;
    for (const Bucket& b : buckets) {
        for (float g : b.grad) {
            ok = ok && g == static_cast<float>(n);
        }
    }
    std::printf("rank %d: finished, every gradient equals %d: %s\n", me, n, ok ? "yes" : "NO");
    MPI_Finalize();
    return ok ? 0 : 1;
}
