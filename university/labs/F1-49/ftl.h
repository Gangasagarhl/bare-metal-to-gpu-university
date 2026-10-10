// ftl.h - the university's tiny flash translation layer (FTL) simulator (chapter F1-49).
// A teaching model with made-up sizes: it is NOT the firmware of any real SSD.
// Rules it models: a flash page can be programmed only when it is free (erased);
// erasing works on a whole block; the FTL therefore writes every new version of a
// logical page to a fresh physical page and later cleans up blocks (garbage collection).
#pragma once
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <optional>
#include <vector>

struct FlashStats
{
    long hostWrites = 0;   // logical pages the host asked to write
    long flashWrites = 0;  // physical pages actually programmed (host + garbage collection)
    long gcCopies = 0;     // valid pages moved by garbage collection
    long erases = 0;       // block erases
};

class TinyFtl
{
public:
    TinyFtl(int blocks, int pagesPerBlock, int logicalPages)
        : ppb_(pagesPerBlock), map_(static_cast<std::size_t>(logicalPages), -1),
          owner_(static_cast<std::size_t>(blocks * pagesPerBlock), -1),
          state_(static_cast<std::size_t>(blocks * pagesPerBlock), Page::Free),
          blocks_(static_cast<std::size_t>(blocks))
    {
        for (int b = blocks - 1; b >= 0; --b) {
            freeList_.push_back(b);
        }
    }

    // The raw flash rule: programming a page that is not erased is refused.
    bool program(int ppn)
    {
        if (state_[idx(ppn)] != Page::Free) {
            return false;
        }
        state_[idx(ppn)] = Page::Valid;
        ++stats_.flashWrites;
        return true;
    }

    void write(int lpn)
    {
        ++stats_.hostWrites;
        invalidate(lpn);
        // Clean only when a new block is needed and just one erased block is left.
        while (needsNewBlock() && freeList_.size() <= kReserve && collectOne()) {
        }
        place(lpn);
    }

    // TRIM: the host says this logical page no longer holds useful data.
    void trim(int lpn) { invalidate(lpn); }

    std::optional<int> lookup(int lpn) const
    {
        const int p = map_[idx(lpn)];
        if (p < 0) {
            return std::nullopt;
        }
        return p;
    }

    const FlashStats& stats() const { return stats_; }
    int freeBlocks() const { return static_cast<int>(freeList_.size()); }
    std::vector<int> eraseCounts() const
    {
        std::vector<int> e;
        for (const auto& b : blocks_) {
            e.push_back(b.eraseCount);
        }
        return e;
    }

private:
    enum class Page { Free, Valid, Invalid };
    struct Block
    {
        int nextPage = 0;    // pages inside a block are programmed in order
        int valid = 0;
        int eraseCount = 0;
    };
    static constexpr std::size_t kReserve = 1;  // erased blocks kept back for cleaning

    static std::size_t idx(int i) { return static_cast<std::size_t>(i); }

    void invalidate(int lpn)
    {
        const int old = map_[idx(lpn)];
        if (old >= 0) {
            state_[idx(old)] = Page::Invalid;
            --blocks_[idx(old / ppb_)].valid;
            owner_[idx(old)] = -1;
            map_[idx(lpn)] = -1;
        }
    }

    bool needsNewBlock() const { return active_ < 0 || blocks_[idx(active_)].nextPage == ppb_; }

    int allocate()
    {
        if (needsNewBlock()) {
            if (freeList_.empty()) {
                std::fprintf(stderr, "internal error: no erased block left\n");
                std::abort();
            }
            active_ = freeList_.back();
            freeList_.pop_back();
        }
        Block& b = blocks_[idx(active_)];
        return active_ * ppb_ + b.nextPage++;
    }

    void place(int lpn)
    {
        const int ppn = allocate();
        if (!program(ppn)) {
            std::fprintf(stderr, "internal error: page %d was not erased\n", ppn);
            return;
        }
        ++blocks_[idx(ppn / ppb_)].valid;
        owner_[idx(ppn)] = lpn;
        map_[idx(lpn)] = ppn;
    }

    // Greedy garbage collection: clean the full block with the fewest valid pages.
    bool collectOne()
    {
        int victim = -1;
        for (int b = 0; b < static_cast<int>(blocks_.size()); ++b) {
            const Block& blk = blocks_[idx(b)];
            const bool full = blk.nextPage == ppb_;
            if (b != active_ && full && (victim < 0 || blk.valid < blocks_[idx(victim)].valid)) {
                victim = b;
            }
        }
        if (victim < 0 || blocks_[idx(victim)].valid == ppb_) {
            return false;  // nothing can be gained
        }
        for (int p = victim * ppb_; p < (victim + 1) * ppb_; ++p) {
            if (state_[idx(p)] == Page::Valid) {
                const int lpn = owner_[idx(p)];
                invalidate(lpn);
                place(lpn);
                ++stats_.gcCopies;
            }
        }
        for (int p = victim * ppb_; p < (victim + 1) * ppb_; ++p) {
            state_[idx(p)] = Page::Free;   // erase: the whole block at once
        }
        Block& v = blocks_[idx(victim)];
        v.nextPage = 0;
        v.valid = 0;
        ++v.eraseCount;
        ++stats_.erases;
        freeList_.insert(freeList_.begin(), victim);
        return true;
    }

    int ppb_;
    std::vector<int> map_;      // logical page -> physical page (-1: none)
    std::vector<int> owner_;    // physical page -> logical page (-1: none)
    std::vector<Page> state_;
    std::vector<Block> blocks_;
    std::vector<int> freeList_;
    int active_ = -1;
    FlashStats stats_;
};

// A small deterministic pseudo-random generator, so every run prints the same numbers.
inline std::uint32_t nextRandom(std::uint32_t& s)
{
    s = s * 1664525u + 1013904223u;
    return s >> 8;
}
