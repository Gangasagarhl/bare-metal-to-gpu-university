// laptop.cpp - BR-10 Listing 1: the job on one machine, measured the F2-51 way.
// Usage: ./laptop [end of range, default 300000] [timed runs, default 21]
#include "bench.hpp"
#include "job.hpp"
#include <cstdio>
#include <cstdlib>
#include <thread>

int main(int argc, char** argv)
{
    std::uint64_t const end = argc > 1 ? std::strtoull(argv[1], nullptr, 10) : 300000;
    int const reps = argc > 2 ? std::atoi(argv[2]) : 21;

    job::Part const answer = job::countPrimes(0, end);  // the reference result
    std::printf("job: primes below %llu -> count %llu, sum %llu\n",
                static_cast<unsigned long long>(end),
                static_cast<unsigned long long>(answer.count),
                static_cast<unsigned long long>(answer.sum));

    bool same = true;
    auto const s = bench::run([&] {
        job::Part const p = job::countPrimes(0, end);
        bench::keep(p.sum);
        same = same && p.count == answer.count && p.sum == answer.sum;
    }, 3, reps);

    std::printf("machine: %u logical CPUs visible; 1 process, 1 thread\n",
                std::thread::hardware_concurrency());
    std::printf("timed runs: %d after 3 warm-ups; every run gave the same answer: %s\n",
                reps, same ? "yes" : "NO");
    std::printf("time in ms: min %.2f  median %.2f  max %.2f  variation %.0f %%\n",
                s.minimum * 1e3, s.median * 1e3, s.maximum * 1e3,
                100.0 * (s.maximum - s.minimum) / s.median);
    return same ? 0 : 1;
}
