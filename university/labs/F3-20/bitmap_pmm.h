// bitmap_pmm.h - F3-20: the simplest frame allocator: one bit per 4 KiB frame (1 = used).
// Host-tested in pmm_host.cpp; the kernel itself uses the buddy allocator (buddy.h).
#pragma once
#include <cstdint>

class BitmapPmm {
public:
    // bits: caller-provided storage for nframes bits, all frames start as used.
    void init(uint64_t* bits, uint64_t nframes)
    {
        bits_ = bits;
        nframes_ = nframes;
        for (uint64_t i = 0; i < (nframes + 63) / 64; ++i) {
            bits_[i] = ~0ull;
        }
        free_ = 0;
    }
    void mark_free(uint64_t first, uint64_t count)
    {
        for (uint64_t f = first; f < first + count; ++f) {
            if (test(f)) {
                clear(f);
                ++free_;
            }
        }
    }
    // Finds 'count' free frames in a row whose first frame number is a multiple of 'align'.
    bool alloc(uint64_t count, uint64_t align, uint64_t& frame)
    {
        for (uint64_t f = 0; f + count <= nframes_; f += align) {
            uint64_t i = 0;
            while (i < count && !test(f + i)) {
                ++i;
            }
            if (i == count) {
                for (i = 0; i < count; ++i) {
                    set(f + i);
                }
                free_ -= count;
                frame = f;
                return true;
            }
        }
        return false;
    }
    bool free(uint64_t frame, uint64_t count)   // false: double free (a frame was not in use)
    {
        for (uint64_t i = 0; i < count; ++i) {
            if (!test(frame + i)) {
                return false;
            }
            clear(frame + i);
            ++free_;
        }
        return true;
    }
    uint64_t free_frames() const { return free_; }

private:
    bool test(uint64_t f) const { return (bits_[f / 64] >> (f % 64)) & 1; }
    void set(uint64_t f) { bits_[f / 64] |= 1ull << (f % 64); }
    void clear(uint64_t f) { bits_[f / 64] &= ~(1ull << (f % 64)); }

    uint64_t* bits_ = nullptr;
    uint64_t nframes_ = 0;
    uint64_t free_ = 0;
};
