// MP5 starter: one acceptance case of milestone 1, run on n threads, with every check the
// suite applies. Used by mp5_suite.cpp (the real algorithm) and mp5_mutants.cpp (known bugs).
#pragma once

#include "mp5_allreduce.h"

#include <cmath>
#include <cstring>
#include <thread>
#include <type_traits>

namespace mp5 {

struct CaseResult
{
    bool status = true;      // every rank returned Status::ok
    bool values = true;      // int: exact sum; float: within the error bound of the reference
    bool identical = true;   // every rank holds the same bits
    bool traffic = true;     // every rank sent exactly 2*count - |seg r+1| - |seg r+2| elements
    double worst = 0.0;      // float: largest |error| / bound seen (must be <= 1)
    bool pass() const { return status && values && identical && traffic; }
};

template <typename T, Mutant M = Mutant::none>
CaseResult runCase(std::size_t n, std::size_t count, std::size_t chunkElems)
{
    std::vector<std::vector<T>> buf(n, std::vector<T>(count));
    for (std::size_t r = 0; r < n; ++r) {
        for (std::size_t i = 0; i < count; ++i) {
            buf[r][i] = inputValue<T>(r, i);
        }
    }
    ThreadRing<T> ring(n, std::chrono::milliseconds(5000));
    std::vector<typename ThreadRing<T>::ThreadLink> links;
    links.reserve(n);
    for (std::size_t r = 0; r < n; ++r) {
        links.emplace_back(ring, r);
    }
    std::vector<Status> status(n, Status::ok);
    std::vector<std::thread> threads;
    for (std::size_t r = 0; r < n; ++r) {
        threads.emplace_back([&, r] {
            status[r] = ringAllReduce<T, M>(links[r], r, n, std::span<T>(buf[r]), chunkElems);
        });
    }
    for (std::thread& t : threads) {
        t.join();
    }

    CaseResult res;
    for (std::size_t r = 0; r < n; ++r) {
        res.status = res.status && status[r] == Status::ok;
        if (n > 1) {
            const Range a = segment(count, n, (r + 1) % n, Mutant::none);
            const Range b = segment(count, n, (r + 2) % n, Mutant::none);
            const std::size_t expect = 2 * count - (a.end - a.begin) - (b.end - b.begin);
            res.traffic = res.traffic && links[r].sentElems == expect;
        }
    }
    for (std::size_t i = 0; i < count; ++i) {
        for (std::size_t r = 1; r < n; ++r) {
            if (std::memcmp(&buf[r][i], &buf[0][i], sizeof(T)) != 0) {
                res.identical = false;
            }
        }
        if constexpr (std::is_integral_v<T>) {
            long exact = 0;
            for (std::size_t r = 0; r < n; ++r) {
                exact += static_cast<long>(inputValue<T>(r, i));
            }
            for (std::size_t r = 0; r < n; ++r) {
                res.values = res.values && static_cast<long>(buf[r][i]) == exact;
            }
        } else {
            // Reference in double; bound for any order of n-1 additions in T:
            // |error| <= (n-1) * u * sum|x|, with u the unit roundoff of T.
            double ref = 0.0;
            double mag = 0.0;
            for (std::size_t r = 0; r < n; ++r) {
                const double x = static_cast<double>(inputValue<T>(r, i));
                ref += x;
                mag += std::fabs(x);
            }
            const double u = std::is_same_v<T, float> ? std::ldexp(1.0, -24) : std::ldexp(1.0, -53);
            const double bound = static_cast<double>(n - 1) * u * mag;
            for (std::size_t r = 0; r < n; ++r) {
                const double err = std::fabs(static_cast<double>(buf[r][i]) - ref);
                if (err > bound) {
                    res.values = false;
                }
                if (bound > 0.0 && err / bound > res.worst) {
                    res.worst = err / bound;
                }
            }
        }
    }
    return res;
}

}  // namespace mp5
