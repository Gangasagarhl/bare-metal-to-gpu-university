// pbt.hpp - the university's tiny property-based testing harness (F12-08).
// for_all() generates random inputs of growing size from a seeded generator, checks a
// property on each, and when one fails, shrinks it: it keeps replacing the failing input with
// a smaller candidate that still fails, until no candidate fails. It prints the seed so any
// failure can be replayed exactly.
#pragma once
#include <cstdint>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <vector>

namespace pbt {

using Rng = std::mt19937_64;  // the C++ standard fixes this engine's output sequence

inline std::uint64_t below(Rng& rng, std::uint64_t n)  // 0 .. n-1 (our own, so it is portable)
{
    return rng() % n;
}

template <class T>
struct Gen {
    std::function<T(Rng&, int size)> make;             // a random value of about this size
    std::function<std::vector<T>(const T&)> shrink;    // smaller candidates, most aggressive first
    std::function<std::string(const T&)> show;         // how to print a value
};

struct Config {
    std::uint64_t seed = 1;
    int runs = 200;
    int max_size = 100;
};

template <class T, class Prop>
bool for_all(const char* name, const Config& cfg, const Gen<T>& gen, Prop prop)
{
    Rng rng(cfg.seed);
    for (int i = 0; i < cfg.runs; ++i) {
        const int size = 1 + (i * cfg.max_size) / cfg.runs;  // small inputs first
        T x = gen.make(rng, size);
        if (prop(x)) {
            continue;
        }
        const std::string original = gen.show(x);
        int steps = 0;
        for (bool smaller = true; smaller;) {
            smaller = false;
            for (const T& c : gen.shrink(x)) {
                if (!prop(c)) {
                    x = c;
                    ++steps;
                    smaller = true;
                    break;
                }
            }
        }
        std::printf("%s: FAILED on run %d of %d (seed %llu)\n", name, i + 1, cfg.runs,
                    static_cast<unsigned long long>(cfg.seed));
        std::printf("  original input: %s\n", original.c_str());
        std::printf("  shrunk input (%d shrink steps): %s\n", steps, gen.show(x).c_str());
        return false;
    }
    std::printf("%s: OK, %d runs passed (seed %llu)\n", name, cfg.runs,
                static_cast<unsigned long long>(cfg.seed));
    return true;
}

// Shrinking helper for any vector: remove a half, then quarters, ..., then single elements.
template <class E>
std::vector<std::vector<E>> remove_chunks(const std::vector<E>& v)
{
    std::vector<std::vector<E>> out;
    for (std::size_t len = v.size() / 2; len >= 1; len /= 2) {
        for (std::size_t at = 0; at + len <= v.size(); at += len) {
            std::vector<E> c(v.begin(), v.begin() + static_cast<std::ptrdiff_t>(at));
            c.insert(c.end(), v.begin() + static_cast<std::ptrdiff_t>(at + len), v.end());
            out.push_back(std::move(c));
        }
    }
    return out;
}

}  // namespace pbt
