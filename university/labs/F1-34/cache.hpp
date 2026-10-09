// A set-associative cache model with least-recently-used (LRU) replacement.
// It keeps only tags (no data): enough to decide hit or miss for every address.
#pragma once
#include <cstdint>
#include <stdexcept>
#include <vector>

struct AccessResult
{
    std::uint64_t set = 0;
    std::uint64_t tag = 0;
    bool hit = false;
    bool evicted = false;          // a valid block had to leave to make room
    std::uint64_t evictedTag = 0;
};

class Cache
{
public:
    Cache(unsigned sets, unsigned ways, unsigned lineBytes)
        : sets_(sets), ways_(ways), lines_(std::vector<Line>(std::size_t{sets} * ways))
    {
        if (!isPowerOfTwo(sets) || !isPowerOfTwo(lineBytes) || ways == 0) {
            throw std::invalid_argument("sets and line size must be powers of two, ways >= 1");
        }
        offsetBits_ = log2(lineBytes);
        indexBits_ = log2(sets);
    }

    AccessResult access(std::uint64_t address)
    {
        AccessResult r;
        r.set = (address >> offsetBits_) & (sets_ - 1);
        r.tag = address >> (offsetBits_ + indexBits_);
        ++clock_;
        Line* victim = &lines_[r.set * ways_];       // candidate to replace on a miss
        for (unsigned w = 0; w < ways_; ++w) {
            Line& line = lines_[r.set * ways_ + w];
            if (line.valid && line.tag == r.tag) {   // hit: refresh its age
                line.lastUse = clock_;
                r.hit = true;
                ++hits_;
                return r;
            }
            if (!line.valid) {
                if (victim->valid) {
                    victim = &line;                  // an empty way is the best victim
                }
            } else if (victim->valid && line.lastUse < victim->lastUse) {
                victim = &line;                      // otherwise the least recently used
            }
        }
        ++misses_;
        if (victim->valid) {
            r.evicted = true;
            r.evictedTag = victim->tag;
        }
        victim->valid = true;
        victim->tag = r.tag;
        victim->lastUse = clock_;
        return r;
    }

    std::uint64_t hits() const { return hits_; }
    std::uint64_t misses() const { return misses_; }

private:
    struct Line
    {
        bool valid = false;
        std::uint64_t tag = 0;
        std::uint64_t lastUse = 0;
    };

    static bool isPowerOfTwo(unsigned x) { return x != 0 && (x & (x - 1)) == 0; }
    static unsigned log2(unsigned x)
    {
        unsigned n = 0;
        while (x > 1) {
            x >>= 1;
            ++n;
        }
        return n;
    }

    unsigned sets_;
    unsigned ways_;
    unsigned offsetBits_ = 0;
    unsigned indexBits_ = 0;
    std::uint64_t clock_ = 0;
    std::uint64_t hits_ = 0;
    std::uint64_t misses_ = 0;
    std::vector<Line> lines_;
};
