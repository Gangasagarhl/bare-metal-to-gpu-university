// memmap.h - F3-20: turn the firmware's memory map into sorted, page-aligned, non-overlapping
// usable ranges, minus the ranges the kernel must keep (its own image, boot data, low memory).
// Pure logic with fixed-size arrays: the same header is unit-tested on the host (pmm_host.cpp).
#pragma once
#include <cstdint>

namespace memmap {

inline constexpr uint64_t kPage = 4096;
inline constexpr int kMaxRanges = 64;

struct Range {
    uint64_t base;
    uint64_t end;       // one past the last byte
};

struct RangeList {
    Range r[kMaxRanges];
    int n = 0;
    uint64_t bytes() const
    {
        uint64_t sum = 0;
        for (int i = 0; i < n; ++i) {
            sum += r[i].end - r[i].base;
        }
        return sum;
    }
};

inline void sort(RangeList& l)       // insertion sort: the lists are short
{
    for (int i = 1; i < l.n; ++i) {
        Range x = l.r[i];
        int j = i - 1;
        while (j >= 0 && l.r[j].base > x.base) {
            l.r[j + 1] = l.r[j];
            --j;
        }
        l.r[j + 1] = x;
    }
}

// Rounds every range inwards to whole pages, drops empty ones, sorts, merges overlaps and
// neighbours. Rounding inwards matters: a partial page at either end is not ours to use.
inline void normalize(RangeList& l)
{
    int k = 0;
    for (int i = 0; i < l.n; ++i) {
        uint64_t b = (l.r[i].base + kPage - 1) & ~(kPage - 1);
        uint64_t e = l.r[i].end & ~(kPage - 1);
        if (e > b) {
            l.r[k++] = Range{b, e};
        }
    }
    l.n = k;
    sort(l);
    k = 0;
    for (int i = 0; i < l.n; ++i) {
        if (k > 0 && l.r[i].base <= l.r[k - 1].end) {
            if (l.r[i].end > l.r[k - 1].end) {
                l.r[k - 1].end = l.r[i].end;
            }
        } else {
            l.r[k++] = l.r[i];
        }
    }
    l.n = k;
}

// Removes [base, end) from every range (splitting a range in two when needed).
inline void subtract(RangeList& l, uint64_t base, uint64_t end)
{
    base &= ~(kPage - 1);                      // rounding outwards: never hand out
    end = (end + kPage - 1) & ~(kPage - 1);    // part of a reserved page
    RangeList out;
    for (int i = 0; i < l.n; ++i) {
        Range x = l.r[i];
        if (x.end <= base || x.base >= end) {
            out.r[out.n++] = x;
            continue;
        }
        if (x.base < base && out.n < kMaxRanges) {
            out.r[out.n++] = Range{x.base, base};
        }
        if (x.end > end && out.n < kMaxRanges) {
            out.r[out.n++] = Range{end, x.end};
        }
    }
    l = out;
}

} // namespace memmap
