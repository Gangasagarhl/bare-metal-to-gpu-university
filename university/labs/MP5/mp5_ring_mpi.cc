// MP5 starter, Listing 4: the SAME ringAllReduce over a second transport, MPI, checked against
// the library's MPI_Allreduce. This is the bridge to milestone 2: the hierarchical version
// (F8-10) uses this link for the inter-node phase. Usage: mpirun -np P ./mp5_ring_mpi
// MPI_Sendrecv sends to the next rank and receives from the previous one in one call, so a
// ring of blocking calls cannot deadlock on large messages (the F8-07 head-to-head lesson).
#include <mpi.h>

#include "mp5_allreduce.h"

#include <cmath>
#include <cstdio>
#include <cstring>

namespace {

template <typename T> MPI_Datatype mpiType();
template <> MPI_Datatype mpiType<std::int32_t>() { return MPI_INT32_T; }
template <> MPI_Datatype mpiType<float>() { return MPI_FLOAT; }

template <typename T>
class MpiLink : public mp5::Link<T>
{
public:
    MpiLink(MPI_Comm comm, int rank, int size) : comm_(comm), next_((rank + 1) % size), prev_((rank + size - 1) % size) {}
    mp5::Status sendRecv(const T* send, std::size_t ns, T* recv, std::size_t nr) override
    {
        MPI_Status st;
        const int rc = MPI_Sendrecv(send, static_cast<int>(ns), mpiType<T>(), next_, 7,
                                    recv, static_cast<int>(nr), mpiType<T>(), prev_, 7, comm_, &st);
        int got = 0;
        MPI_Get_count(&st, mpiType<T>(), &got);
        if (rc != MPI_SUCCESS || static_cast<std::size_t>(got) != nr) {
            return mp5::Status::aborted;          // with MPI_ERRORS_ARE_FATAL we never get here
        }
        this->sentElems += ns;
        return mp5::Status::ok;
    }

private:
    MPI_Comm comm_;
    int next_;
    int prev_;
};

std::uint64_t fnv(const void* p, std::size_t n)
{
    std::uint64_t h = 1469598103934665603ull;
    const unsigned char* b = static_cast<const unsigned char*>(p);
    for (std::size_t i = 0; i < n; ++i) {
        h = (h ^ b[i]) * 1099511628211ull;
    }
    return h;
}

// returns 1 if this case passed on every rank
template <typename T>
int check(int rank, int size, std::size_t count, std::size_t chunk, long& bitsDiffer)
{
    std::vector<T> mine(count);
    for (std::size_t i = 0; i < count; ++i) {
        mine[i] = mp5::inputValue<T>(static_cast<std::size_t>(rank), i);
    }
    std::vector<T> lib(count);
    MPI_Allreduce(mine.data(), lib.data(), static_cast<int>(count), mpiType<T>(), MPI_SUM, MPI_COMM_WORLD);
    MpiLink<T> link(MPI_COMM_WORLD, rank, size);
    const mp5::Status st = mp5::ringAllReduce<T>(link, static_cast<std::size_t>(rank), static_cast<std::size_t>(size),
                                                 std::span<T>(mine), chunk);
    int ok = st == mp5::Status::ok ? 1 : 0;
    long differ = 0;
    for (std::size_t i = 0; i < count; ++i) {
        if constexpr (std::is_integral_v<T>) {
            ok = ok && mine[i] == lib[i];
        } else {
            double ref = 0.0;
            double mag = 0.0;
            for (int r = 0; r < size; ++r) {
                const double x = static_cast<double>(mp5::inputValue<T>(static_cast<std::size_t>(r), i));
                ref += x;
                mag += std::fabs(x);
            }
            const double bound = (size - 1) * std::ldexp(1.0, -24) * mag;
            ok = ok && std::fabs(static_cast<double>(mine[i]) - ref) <= bound;
            differ += std::memcmp(&mine[i], &lib[i], sizeof(T)) != 0 ? 1 : 0;
        }
    }
    // every rank must hold the same bits
    const std::uint64_t h = fnv(mine.data(), count * sizeof(T));
    std::vector<std::uint64_t> all(static_cast<std::size_t>(size));
    MPI_Allgather(&h, 1, MPI_UINT64_T, all.data(), 1, MPI_UINT64_T, MPI_COMM_WORLD);
    for (std::uint64_t x : all) {
        ok = ok && x == all[0];
    }
    int okAll = 0;
    MPI_Allreduce(&ok, &okAll, 1, MPI_INT, MPI_MIN, MPI_COMM_WORLD);
    long differAll = 0;
    MPI_Reduce(&differ, &differAll, 1, MPI_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
    bitsDiffer += differAll;
    return okAll;
}

}  // namespace

int main(int argc, char** argv)
{
    MPI_Init(&argc, &argv);
    int rank = 0;
    int size = 0;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int cases = 0;
    int passed = 0;
    long bitsDiffer = 0;
    long floatValues = 0;
    for (std::size_t count : {0u, 1u, 2u, 3u, 7u, 40u, 1000u, 4099u, 65539u, 1048576u}) {
        for (std::size_t chunk : {3u, 1024u, 16384u}) {
            if (chunk == 3 && count > 4099) {
                continue;
            }
            passed += check<std::int32_t>(rank, size, count, chunk, bitsDiffer);
            passed += check<float>(rank, size, count, chunk, bitsDiffer);
            cases += 2;
            floatValues += static_cast<long>(count) * size;
        }
    }
    if (rank == 0) {
        std::printf("P=%d MPI processes: %d of %d cases PASS (int32 equal to MPI_Allreduce; float within bound, "
                    "ranks bit-identical); float values that differ in bits from MPI_Allreduce: %ld of %ld\n",
                    size, passed, cases, bitsDiffer, floatValues);
    }
    MPI_Finalize();
    return passed == cases ? 0 : 1;
}
