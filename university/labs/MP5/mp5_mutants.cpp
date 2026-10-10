// MP5 starter, Listing 2: does the suite have teeth? Each mutant is the real ring with one
// known bug (mp5_allreduce.h, enum Mutant). A smaller version of the suite runs against each
// one and reports which check failed first and how many cases caught it. A mutant that no
// case catches ("SURVIVED") would mean the suite is too weak for the correctness rubric.
// For contrast, a "naive" suite (power-of-two sizes only, values only) runs as well.
#include "mp5_check.h"

#include <cstdio>
#include <string>

namespace {

struct Score
{
    int cases = 0;
    int caught = 0;
    std::string first;
    int naiveCaught = 0;     // cases of the naive suite that saw wrong values
};

std::string why(const mp5::CaseResult& r)
{
    if (!r.status) {
        return "status (a rank returned an error)";
    }
    if (!r.traffic) {
        return "traffic (elements sent per rank)";
    }
    if (!r.values) {
        return "values (sum)";
    }
    return "bit-identical ranks";
}

template <mp5::Mutant M>
Score attack()
{
    Score s;
    for (std::size_t n : {2u, 3u, 4u, 8u}) {
        for (std::size_t count : {1u, 2u, 3u, 7u, 8u, 9u, 64u, 1000u, 4099u}) {
            for (std::size_t chunk : {3u, 1024u}) {
                const mp5::CaseResult ri = mp5::runCase<std::int32_t, M>(n, count, chunk);
                const mp5::CaseResult rf = mp5::runCase<float, M>(n, count, chunk);
                s.cases += 2;
                for (const mp5::CaseResult* r : {&ri, &rf}) {
                    if (!r->pass()) {
                        ++s.caught;
                        if (s.first.empty()) {
                            s.first = "N=" + std::to_string(n) + " count=" + std::to_string(count) +
                                      " chunk=" + std::to_string(chunk) + ": " + why(*r);
                        }
                    }
                }
            }
        }
    }
    for (std::size_t n : {2u, 4u, 8u}) {            // the naive suite: 1024 and 4096 elements
        for (std::size_t count : {1024u, 4096u}) {
            const mp5::CaseResult r = mp5::runCase<std::int32_t, M>(n, count, 1024);
            s.naiveCaught += (!r.status || !r.values) ? 1 : 0;
        }
    }
    return s;
}

void report(const char* name, const Score& s, bool expectCaught, bool& ok)
{
    const bool caught = s.caught > 0;
    std::printf("%-14s naive suite: %d of 6 | full suite: %3d of %3d cases; %s%s\n", name, s.naiveCaught,
                s.caught, s.cases, caught ? "KILLED, first: " : (expectCaught ? "SURVIVED" : "no case fails (expected)"),
                caught ? s.first.c_str() : "");
    ok = ok && (caught == expectCaught);
}

}  // namespace

int main()
{
    bool ok = true;
    report("none (real)", attack<mp5::Mutant::none>(), false, ok);
    report("gather_short", attack<mp5::Mutant::gather_short>(), true, ok);
    report("chunks_floor", attack<mp5::Mutant::chunks_floor>(), true, ok);
    report("split_floor", attack<mp5::Mutant::split_floor>(), true, ok);
    report("recv_segment", attack<mp5::Mutant::recv_segment>(), true, ok);
    std::printf("mutation check: %s\n", ok ? "every mutant killed, the real ring passes" : "UNEXPECTED");
    return ok ? 0 : 1;
}
