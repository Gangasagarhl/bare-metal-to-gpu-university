// HW203 course project, Lab Engineer's reference (guide 11.5): "a cache simulator in C++ validated
// against hand traces". Proves the project is feasible: configurable sets/ways/line size, LRU or
// FIFO replacement, reads and writes with write-back + write-allocate, a dirty bit and a final
// flush, and an automatic check of every hand trace against its expected hit/miss string and counts.
// Input (project_ref.in): blocks of
//   trace <name> <sets> <ways> <lineBytes> <LRU|FIFO>
//   expect <string of h/M, one letter per access> <lineReads> <writeBacks>
//   <R|W> <hex address>      (one line per access)
//   end
// The expected strings were written by hand before the run (traces 1-3 are F1-33's cachesim.in in
// its three geometries, trace 4 is F1-35's writepolicy.in, traces 5-6 are FIFO versus LRU).
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

class Sim
{
public:
    Sim(unsigned sets, unsigned ways, unsigned lineBytes, bool fifo)
        : sets_(sets), ways_(ways), lineBytes_(lineBytes), fifo_(fifo), lines_(std::size_t{sets} * ways)
    {
        if (sets == 0 || (sets & (sets - 1)) != 0 || lineBytes == 0 || (lineBytes & (lineBytes - 1)) != 0 ||
            ways == 0) {
            throw std::invalid_argument("sets and line size must be powers of two, ways >= 1");
        }
    }

    // Returns true on a hit. Write-back + write-allocate: a write miss loads the line first.
    bool access(char kind, std::uint64_t address)
    {
        const std::uint64_t block = address / lineBytes_;
        const std::uint64_t set = block % sets_;
        const std::uint64_t tag = block / sets_;
        ++clock_;
        Line* base = &lines_[set * ways_];
        for (unsigned w = 0; w < ways_; ++w) {
            Line& l = base[w];
            if (l.valid && l.tag == tag) {
                if (!fifo_) {
                    l.stamp = clock_;   // LRU refreshes on use; FIFO keeps the fill time
                }
                if (kind == 'W') {
                    l.dirty = true;
                }
                return true;
            }
        }
        Line* victim = nullptr;
        for (unsigned w = 0; w < ways_ && victim == nullptr; ++w) {
            if (!base[w].valid) {
                victim = &base[w];
            }
        }
        if (victim == nullptr) {
            victim = base;
            for (unsigned w = 1; w < ways_; ++w) {
                if (base[w].stamp < victim->stamp) {
                    victim = &base[w];
                }
            }
            if (victim->dirty) {
                ++writeBacks_;
            }
        }
        ++lineReads_;
        *victim = Line{true, kind == 'W', tag, clock_};
        return false;
    }

    void flush()
    {
        for (Line& l : lines_) {
            if (l.valid && l.dirty) {
                ++writeBacks_;
                l.dirty = false;
            }
        }
    }

    int lineReads() const { return lineReads_; }
    int writeBacks() const { return writeBacks_; }

private:
    struct Line
    {
        bool valid = false;
        bool dirty = false;
        std::uint64_t tag = 0;
        std::uint64_t stamp = 0;   // last use (LRU) or fill time (FIFO)
    };
    unsigned sets_, ways_, lineBytes_;
    bool fifo_;
    std::vector<Line> lines_;
    std::uint64_t clock_ = 0;
    int lineReads_ = 0;
    int writeBacks_ = 0;
};

int main()
{
    std::string word;
    int traces = 0;
    int failures = 0;
    while (std::cin >> word) {
        if (word != "trace") {
            std::printf("bad input near '%s'\n", word.c_str());
            return 2;
        }
        std::string name, policy, expected;
        unsigned sets = 0, ways = 0, lineBytes = 0;
        int expReads = 0, expWriteBacks = 0;
        std::cin >> name >> sets >> ways >> lineBytes >> policy >> word >> expected >> expReads >> expWriteBacks;
        Sim sim(sets, ways, lineBytes, policy == "FIFO");
        std::string got;
        std::string kind;
        while (std::cin >> kind && kind != "end") {
            std::string hex;
            std::cin >> hex;
            got += sim.access(kind[0], std::stoull(hex, nullptr, 16)) ? 'h' : 'M';
        }
        sim.flush();
        const bool ok = got == expected && sim.lineReads() == expReads && sim.writeBacks() == expWriteBacks;
        std::printf("%-14s %3u sets x %2u ways x %3u B %-4s  got %-16s reads %d write-backs %d  %s\n",
                    name.c_str(), sets, ways, lineBytes, policy.c_str(), got.c_str(), sim.lineReads(),
                    sim.writeBacks(), ok ? "ok" : "MISMATCH");
        if (!ok) {
            std::printf("    expected %s reads %d write-backs %d\n", expected.c_str(), expReads, expWriteBacks);
            ++failures;
        }
        ++traces;
    }
    std::printf("%d hand traces, %d mismatches\n", traces, failures);
    return failures == 0 ? 0 : 1;
}
