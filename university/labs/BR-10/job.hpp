// job.hpp - the job of BR-10: count the primes in [0, end) and add them up.
// The same function runs on the laptop (laptop.cpp) and on every node of the cluster
// (cluster.hpp, job_mpi.cc); only where it runs changes.
#pragma once
#include <cstdint>

namespace job
{

struct Part
{
    std::uint64_t count = 0;  // how many primes
    std::uint64_t sum = 0;    // their sum
};

inline bool isPrime(std::uint64_t n)
{
    if (n < 2) {
        return false;
    }
    if (n % 2 == 0) {
        return n == 2;
    }
    for (std::uint64_t d = 3; d * d <= n; d += 2) {
        if (n % d == 0) {
            return false;
        }
    }
    return true;
}

// One piece of the job: the numbers lo, lo + 1, ..., hi - 1.
inline Part countPrimes(std::uint64_t lo, std::uint64_t hi)
{
    Part p;
    for (std::uint64_t n = lo; n < hi; ++n) {
        if (isPrime(n)) {
            ++p.count;
            p.sum += n;
        }
    }
    return p;
}

// Task t of 'tasks' equal ranges. Larger numbers cost more, so later tasks are slower.
inline std::uint64_t taskBegin(int t, int tasks, std::uint64_t end)
{
    return end * static_cast<std::uint64_t>(t) / static_cast<std::uint64_t>(tasks);
}

}  // namespace job
