// MP5 forensic lab evidence generator: "build B" of the team's ring all-reduce, run on 4 threads
// with int32 data for the sizes of the CI suite (powers of two) and for the sizes of the model
// that failed. The chapter shows only this program's output; the answer key names the bug.
#include "mp5_allreduce.h"

#include <cstdio>
#include <string>
#include <thread>

namespace {

constexpr std::size_t kN = 4;
constexpr mp5::Mutant kBuildB = mp5::Mutant::split_floor;

void evidence(std::size_t count)
{
    std::vector<std::vector<std::int32_t>> buf(kN, std::vector<std::int32_t>(count));
    for (std::size_t r = 0; r < kN; ++r) {
        for (std::size_t i = 0; i < count; ++i) {
            buf[r][i] = mp5::inputValue<std::int32_t>(r, i);
        }
    }
    mp5::ThreadRing<std::int32_t> ring(kN, std::chrono::milliseconds(5000));
    std::vector<mp5::ThreadRing<std::int32_t>::ThreadLink> links;
    links.reserve(kN);
    for (std::size_t r = 0; r < kN; ++r) {
        links.emplace_back(ring, r);
    }
    std::vector<mp5::Status> st(kN);
    std::vector<std::thread> th;
    for (std::size_t r = 0; r < kN; ++r) {
        th.emplace_back([&, r] {
            st[r] = mp5::ringAllReduce<std::int32_t, kBuildB>(links[r], r, kN, std::span<std::int32_t>(buf[r]), 1024);
        });
    }
    for (std::thread& t : th) {
        t.join();
    }
    std::string statuses;
    std::string sent;
    for (std::size_t r = 0; r < kN; ++r) {
        statuses += std::string(r ? "," : "") + mp5::name(st[r]);
        sent += (r ? "," : "") + std::to_string(links[r].sentElems);
    }
    std::size_t wrong = 0;
    std::string where;
    int shown = 0;
    for (std::size_t i = 0; i < count; ++i) {
        long exact = 0;
        for (std::size_t r = 0; r < kN; ++r) {
            exact += mp5::inputValue<std::int32_t>(r, i);
        }
        bool bad = false;
        for (std::size_t r = 0; r < kN; ++r) {
            bad = bad || buf[r][i] != exact;
        }
        if (bad) {
            ++wrong;
            if (shown < 6) {
                where += " " + std::to_string(i) + "(rank0=" + std::to_string(buf[0][i]) + ",want=" + std::to_string(exact) + ")";
                ++shown;
            }
        }
    }
    std::printf("count %5zu: status %s | elements sent per rank %s | wrong elements %zu%s%s\n", count,
                statuses.c_str(), sent.c_str(), wrong, wrong ? " at" : "", where.c_str());
}

}  // namespace

int main()
{
    std::printf("build B, 4 ranks, int32 sum, chunk 1024 elements\n");
    std::printf("-- sizes of the CI suite\n");
    for (std::size_t c : {1024u, 4096u, 65536u}) {
        evidence(c);
    }
    std::printf("-- sizes from the failing model\n");
    for (std::size_t c : {1000u, 1001u, 1002u, 1003u, 4099u, 3u}) {
        evidence(c);
    }
    return 0;
}
