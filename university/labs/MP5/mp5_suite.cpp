// MP5 starter, Listing 1: the milestone-1 acceptance suite ("single-node ring all-reduce
// correct for all sizes"), scaled down to CPU threads. For every rank count, element count,
// chunk size and data type it checks: status, values, bit-identical ranks, exact traffic.
// Sizes: every count from 0 to 40 elements (all remainders mod n), awkward primes, and
// 1 MiB / 16 MiB of 4-byte data. 1 GiB and real GPUs are for the hardware run (handbook).
#include "mp5_check.h"

#include <cstdio>
#include <string>

namespace {

struct Tally
{
    int cases = 0;
    int passed = 0;
    double worst = 0.0;
    std::string firstFail;
};

template <typename T>
void runAll(const char* type, std::size_t n, Tally& t)
{
    std::vector<std::size_t> counts;
    for (std::size_t c = 0; c <= 40; ++c) {
        counts.push_back(c);                       // every remainder modulo every n
    }
    for (std::size_t c : {101u, 257u, 1000u, 4099u, 65539u}) {
        counts.push_back(c);
    }
    counts.push_back(std::size_t{1} << 18);        // 1 MiB of 4-byte data
    counts.push_back(std::size_t{1} << 22);        // 16 MiB of 4-byte data
    for (std::size_t count : counts) {
        for (std::size_t chunk : {std::size_t{3}, std::size_t{1024}, std::size_t{16384}}) {
            if (chunk == 3 && count > 4099) {
                continue;                          // tiny chunks on big data: too slow under sanitizers
            }
            const mp5::CaseResult r = mp5::runCase<T>(n, count, chunk);
            ++t.cases;
            t.passed += r.pass() ? 1 : 0;
            t.worst = r.worst > t.worst ? r.worst : t.worst;
            if (!r.pass() && t.firstFail.empty()) {
                t.firstFail = std::string(type) + " count " + std::to_string(count) + " chunk " + std::to_string(chunk);
            }
        }
    }
}

}  // namespace

int main()
{
    bool all = true;
    std::printf("%-6s %3s %6s %6s %s\n", "type", "N", "cases", "pass", "largest float error / bound");
    for (std::size_t n : {1u, 2u, 3u, 4u, 5u, 8u}) {
        Tally ti;
        Tally tf;
        runAll<std::int32_t>("int32", n, ti);
        runAll<float>("float", n, tf);
        std::printf("%-6s %3zu %6d %6d %s\n", "int32", n, ti.cases, ti.passed, "-");
        std::printf("%-6s %3zu %6d %6d %.3f\n", "float", n, tf.cases, tf.passed, tf.worst);
        for (const Tally* t : {&ti, &tf}) {
            if (t->passed != t->cases) {
                all = false;
                std::printf("  first failure: %s\n", t->firstFail.c_str());
            }
        }
    }
    std::printf("checks per case: every rank returned ok; int32 equals the exact sum; float within "
                "(N-1)*2^-24*sum|x| of a double reference; all ranks bit-identical; each rank sent exactly "
                "2*count - |segment r+1| - |segment r+2| elements\n");
    std::printf("milestone 1 (CPU stand-in): %s\n", all ? "all cases PASS" : "FAILURES");
    return all ? 0 : 1;
}
