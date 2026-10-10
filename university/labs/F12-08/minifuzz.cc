// minifuzz.cc - the university's tiny mutation fuzzer (F12-08).
//   minifuzz guided <seed> <max executions>   keep inputs that reach new code (coverage-guided)
//   minifuzz blind  <seed> <max executions>   mutate the starting input only, no feedback
// The target (pkt.cc) is compiled with -fsanitize-coverage=trace-pc: the compiler inserts a call
// to __sanitizer_cov_trace_pc() at the start of every basic block, and this file defines that
// function to record which blocks ran. AddressSanitizer reports memory errors; our death
// callback then prints the input that caused it.
#include "pkt.hh"

#include <sanitizer/common_interface_defs.h>

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

namespace {

std::array<std::uint8_t, 4096> g_hits{};  // which basic blocks ran (hashed program counters)
const std::vector<std::uint8_t>* g_current = nullptr;
long g_exec = 0;

void print_hex(const std::vector<std::uint8_t>& in)
{
    for (std::size_t i = 0; i < in.size(); ++i) {
        std::fprintf(stderr, "%02x%s", in[i], (i % 16 == 15) ? "\n" : " ");
    }
    std::fprintf(stderr, "\n");
}

void on_death()
{
    if (g_current != nullptr) {
        std::fprintf(stderr, "minifuzz: crash on execution %ld; input (%zu bytes, hex):\n",
                     g_exec, g_current->size());
        print_hex(*g_current);
    }
}

int run_one(const std::vector<std::uint8_t>& in)  // returns how many new blocks it reached
{
    std::array<std::uint8_t, 4096> before = g_hits;
    g_current = &in;
    ++g_exec;
    Frame f;
    parse_frame(in.data(), in.size(), f);
    g_current = nullptr;
    int fresh = 0;
    for (std::size_t i = 0; i < g_hits.size(); ++i) {
        fresh += (g_hits[i] != 0 && before[i] == 0) ? 1 : 0;
    }
    return fresh;
}

void mutate(std::vector<std::uint8_t>& v, std::mt19937& rng)
{
    const int n = 1 + static_cast<int>(rng() % 4);
    for (int k = 0; k < n; ++k) {
        const std::size_t at = v.empty() ? 0 : rng() % v.size();
        const auto pos = v.begin() + static_cast<std::ptrdiff_t>(at);
        switch (rng() % 5) {
        case 0:  // flip one bit
            if (!v.empty()) {
                v[at] ^= static_cast<std::uint8_t>(1u << (rng() % 8));
            }
            break;
        case 1:  // set a random byte
            if (!v.empty()) {
                v[at] = static_cast<std::uint8_t>(rng());
            }
            break;
        case 2:  // insert a random byte
            if (v.size() < 300) {
                v.insert(pos, static_cast<std::uint8_t>(rng()));
            }
            break;
        case 3:  // erase a byte
            if (v.size() > 1) {
                v.erase(pos);
            }
            break;
        default:  // append a copy of a chunk (grows inputs quickly)
            if (!v.empty() && v.size() < 300) {
                const std::size_t len = 1 + rng() % v.size();
                const std::size_t from = rng() % (v.size() - len + 1);
                const auto first = v.begin() + static_cast<std::ptrdiff_t>(from);
                std::vector<std::uint8_t> chunk(first, first + static_cast<std::ptrdiff_t>(len));
                v.insert(v.end(), chunk.begin(), chunk.end());
            }
            break;
        }
    }
}

}  // namespace

extern "C" void __sanitizer_cov_trace_pc()
{
    const auto pc = reinterpret_cast<std::uintptr_t>(__builtin_return_address(0));
    g_hits[(pc ^ (pc >> 12)) % g_hits.size()] = 1;
}

int main(int argc, char** argv)
{
    if (argc != 4) {
        std::fprintf(stderr, "usage: minifuzz guided|blind <seed> <max executions>\n");
        return 2;
    }
    const std::string mode = argv[1];
    const bool guided = mode == "guided";
    std::mt19937 rng(static_cast<std::uint32_t>(std::strtoul(argv[2], nullptr, 10)));
    const long max_exec = std::strtol(argv[3], nullptr, 10);
    __sanitizer_set_death_callback(on_death);
    std::setvbuf(stdout, nullptr, _IOLBF, 0);  // a crash aborts the process: flush every line

    std::vector<std::vector<std::uint8_t>> corpus{{'h', 'e', 'l', 'l', 'o'}};  // one dull seed
    run_one(corpus[0]);
    std::printf("minifuzz %s: seed %s, start corpus 1 input\n", mode.c_str(), argv[2]);
    while (g_exec < max_exec) {
        std::vector<std::uint8_t> in = corpus[guided ? rng() % corpus.size() : 0];
        mutate(in, rng);
        if (run_one(in) > 0 && guided) {
            corpus.push_back(in);
            std::printf("  exec %8ld  new coverage, corpus %zu, input %zu bytes, first bytes",
                        g_exec, corpus.size(), in.size());
            for (std::size_t i = 0; i < 4 && i < in.size(); ++i) {
                std::printf(" %02x", in[i]);
            }
            std::printf("\n");
        }
    }
    std::printf("minifuzz %s: %ld executions, no crash, corpus %zu inputs\n", mode.c_str(),
                g_exec, corpus.size());
    return 0;
}
