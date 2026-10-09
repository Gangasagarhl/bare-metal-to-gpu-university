// fuzz.cpp: the university's small mutation fuzzer for elf_reader.h (milestone P2 asks for a
// fuzzing run of at least an hour that finds nothing). It is not coverage-guided: it takes
// real ELF files as seeds, damages copies of them at random, and makes the reader walk
// everything (headers, names, section contents, symbols). Built with AddressSanitizer and
// UndefinedBehaviorSanitizer, any out-of-bounds read or undefined behaviour stops the run;
// the input that caused it is saved to crash-input.bin first.
// Usage: fuzz SECONDS SEED_FILE...
#include "elf_reader.h"

#include <sanitizer/common_interface_defs.h>

#include <array>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <random>
#include <string>
#include <vector>

namespace {

std::vector<std::byte> current;   // the input being tested, saved if a sanitizer fires

void saveCurrent()
{
    std::ofstream out("crash-input.bin", std::ios::binary);
    out.write(reinterpret_cast<const char*>(current.data()),
              static_cast<std::streamsize>(current.size()));
    std::fprintf(stderr, "fuzz: sanitizer fired; input saved to crash-input.bin\n");
}

// Walk everything elfread walks; return the first error (or ok).
elf::Error exercise(std::span<const std::byte> bytes, std::uint64_t& checksum)
{
    elf::File f;
    if (elf::Error e = elf::File::open(bytes, f); e != elf::Error::ok) return e;
    for (std::uint64_t i = 0; i < f.segmentCount(); ++i) {
        elf::Phdr p{};
        if (elf::Error e = f.segment(i, p); e != elf::Error::ok) return e;
        checksum += p.vaddr + p.memsz;
    }
    for (std::uint64_t i = 0; i < f.sectionCount(); ++i) {
        elf::Shdr s{};
        if (elf::Error e = f.section(i, s); e != elf::Error::ok) return e;
        std::string_view name;
        if (elf::Error e = f.sectionName(s, name); e != elf::Error::ok) return e;
        checksum += name.size();
        std::span<const std::byte> data;
        if (elf::Error e = f.contents(s, data); e != elf::Error::ok) return e;
        if (!data.empty()) checksum += std::to_integer<unsigned>(data.back());
        if (s.type != elf::SHT_SYMTAB && s.type != elf::SHT_DYNSYM) continue;
        std::uint64_t n = 0;
        if (elf::Error e = f.symbolCount(s, n); e != elf::Error::ok) return e;
        for (std::uint64_t k = 0; k < n; ++k) {
            elf::Sym y{};
            if (elf::Error e = f.symbol(s, k, y); e != elf::Error::ok) return e;
            std::string_view sym;
            if (elf::Error e = f.string(s.link, y.name, sym); e != elf::Error::ok) return e;
            checksum += sym.size() + y.value;
        }
    }
    return elf::Error::ok;
}

std::vector<std::byte> load(const char* path)
{
    std::ifstream in(path, std::ios::binary);
    const std::vector<char> raw{std::istreambuf_iterator<char>(in),
                                std::istreambuf_iterator<char>()};
    std::vector<std::byte> out(raw.size());
    std::memcpy(out.data(), raw.data(), raw.size());
    return out;
}

}  // namespace

int main(int argc, char** argv)
{
    if (argc < 3) {
        std::fprintf(stderr, "usage: fuzz SECONDS SEED_FILE...\n");
        return 2;
    }
    const long seconds = std::strtol(argv[1], nullptr, 10);
    std::vector<std::vector<std::byte>> seeds;
    for (int i = 2; i < argc; ++i) {
        seeds.push_back(load(argv[i]));
        std::printf("seed %d: %s (%zu bytes)\n", i - 1, argv[i], seeds.back().size());
    }
    __sanitizer_set_death_callback(saveCurrent);

    std::mt19937_64 rng(20261009);   // fixed seed: the same run can be repeated exactly
    // "Interesting" values: zero, all ones, sign boundaries and sizes near the file size.
    static constexpr std::array<std::uint64_t, 10> interesting = {
        0, 1, 0x7f, 0xff, 0x7fff, 0xffff, 0x7fffffff, 0xffffffff, 0x7fffffffffffffffULL,
        0xffffffffffffffffULL};
    std::map<std::string, std::uint64_t> results;
    std::uint64_t iterations = 0, checksum = 0;
    const auto start = std::chrono::steady_clock::now();
    const auto deadline = start + std::chrono::seconds(seconds);

    while (std::chrono::steady_clock::now() < deadline) {
        for (int batch = 0; batch < 256; ++batch, ++iterations) {
            current = seeds[rng() % seeds.size()];
            const int mutations = 1 + static_cast<int>(rng() % 8);
            for (int m = 0; m < mutations && !current.empty(); ++m) {
                // Bias half of the mutations to the first 64 bytes and the header tables,
                // where one changed byte reaches the most code.
                const std::size_t hot = std::min<std::size_t>(current.size(), 4096);
                const std::size_t pos = (rng() & 1) ? rng() % hot : rng() % current.size();
                switch (rng() % 5) {
                case 0:   // flip one bit
                    current[pos] ^= std::byte{static_cast<unsigned char>(1u << (rng() % 8))};
                    break;
                case 1: current[pos] = std::byte{static_cast<unsigned char>(rng())}; break;
                case 2: {   // overwrite 2, 4 or 8 bytes with an interesting value
                    const std::size_t width = std::size_t{2} << (rng() % 3);
                    const std::uint64_t v = interesting[rng() % interesting.size()];
                    if (pos + width <= current.size()) std::memcpy(current.data() + pos, &v, width);
                    break;
                }
                case 3: current.resize(rng() % (current.size() + 1)); break;   // truncate
                default: {  // copy a random 8-byte block from elsewhere in the file
                    const std::size_t from = rng() % current.size();
                    if (pos + 8 <= current.size() && from + 8 <= current.size()) {
                        std::memmove(current.data() + pos, current.data() + from, 8);
                    }
                    break;
                }
                }
            }
            ++results[elf::message(exercise(current, checksum))];
        }
    }
    const auto secs = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::steady_clock::now() - start).count();
    std::printf("ran %lld s, %llu inputs, no sanitizer report, no crash\n",
                static_cast<long long>(secs), static_cast<unsigned long long>(iterations));
    std::printf("results by outcome (the reader's own messages):\n");
    for (const auto& [what, count] : results) {
        std::printf("  %10llu  %s\n", static_cast<unsigned long long>(count), what.c_str());
    }
    std::printf("(checksum %llu, printed only so the compiler cannot skip the work)\n",
                static_cast<unsigned long long>(checksum));
    return 0;
}
