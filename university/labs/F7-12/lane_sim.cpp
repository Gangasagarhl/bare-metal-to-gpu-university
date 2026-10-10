// F7-12 Listing 1: a lock-step wave simulator for cross-lane operations.
// Every function takes the values of ALL lanes at once, as the hardware does.
#include <bit>
#include <cstdint>
#include <cstdio>
#include <vector>

using Lanes = std::vector<int>;

// ballot: bit l of the result is set when lane l's predicate is true (64-bit result).
std::uint64_t ballot(const std::vector<bool>& pred)
{
    std::uint64_t m = 0;
    for (std::size_t l = 0; l < pred.size(); ++l) {
        if (pred[l]) {
            m |= std::uint64_t{1} << l;
        }
    }
    return m;
}

// shuffle down by `delta` inside the wave; a lane whose source is outside keeps its value.
Lanes shflDown(const Lanes& v, int delta)
{
    Lanes r(v);
    for (std::size_t l = 0; l + static_cast<std::size_t>(delta) < v.size(); ++l) {
        r[l] = v[l + static_cast<std::size_t>(delta)];
    }
    return r;
}

// shuffle xor: lane l reads lane l ^ mask (a butterfly exchange).
Lanes shflXor(const Lanes& v, int mask)
{
    Lanes r(v.size());
    for (std::size_t l = 0; l < v.size(); ++l) {
        r[l] = v[l ^ static_cast<std::size_t>(mask)];
    }
    return r;
}

int reduceDown(Lanes v, int firstDelta)
{
    for (int d = firstDelta; d > 0; d /= 2) {
        const Lanes o = shflDown(v, d);
        for (std::size_t l = 0; l < v.size(); ++l) {
            v[l] += o[l];
        }
    }
    return v[0];
}

// Stream compaction: lane l with a true predicate writes to slot = number of true lanes below l.
// `maskBits` = 64 keeps the whole ballot; 32 imitates storing it in a 32-bit variable.
std::vector<int> compactSlots(const std::vector<bool>& pred, int maskBits)
{
    std::uint64_t m = ballot(pred);
    if (maskBits == 32) {
        m = static_cast<std::uint32_t>(m);                       // the truncation bug
    }
    std::vector<int> slot(pred.size(), -1);
    for (std::size_t l = 0; l < pred.size(); ++l) {
        if (pred[l]) {
            const std::uint64_t below = (l == 0) ? 0 : (m & ((std::uint64_t{1} << l) - 1));
            slot[l] = std::popcount(below);                       // __popcll(mask & lanemask_lt)
        }
    }
    return slot;
}

int main()
{
    const int w = 64;
    Lanes v(w);
    std::vector<bool> pred(w);
    for (int l = 0; l < w; ++l) {
        v[static_cast<std::size_t>(l)] = l + 1;
        pred[static_cast<std::size_t>(l)] = (l % 3 == 0);       // lanes 0, 3, 6, ... 63
    }

    std::printf("-- ballot of (lane %% 3 == 0) on a 64-lane wave\n");
    const std::uint64_t m = ballot(pred);
    std::printf("mask = 0x%016llx, popcount %d\n", static_cast<unsigned long long>(m), std::popcount(m));
    const std::uint32_t m32 = static_cast<std::uint32_t>(m);
    std::printf("same mask kept in 32 bits = 0x%08x, popcount %d (lanes 32..63 lost)\n", m32, std::popcount(m32));

    std::printf("-- reduction of 1..64 (expected %d)\n", w * (w + 1) / 2);
    std::printf("offsets 16,8,4,2,1 (copied from 32-lane code): lane 0 = %d\n", reduceDown(v, 16));
    std::printf("offsets 32,16,8,4,2,1 (warpSize / 2 first):     lane 0 = %d\n", reduceDown(v, w / 2));

    std::printf("-- butterfly (shfl_xor 32,16,...,1): every lane ends with the total\n");
    Lanes b(v);
    for (int mask = w / 2; mask > 0; mask /= 2) {
        const Lanes o = shflXor(b, mask);
        for (std::size_t l = 0; l < b.size(); ++l) {
            b[l] += o[l];
        }
    }
    std::printf("lane 0 = %d, lane 37 = %d, lane 63 = %d\n", b[0], b[37], b[63]);

    std::printf("-- compaction slots for the true lanes (lane:slot)\n");
    for (int bits : {64, 32}) {
        const std::vector<int> s = compactSlots(pred, bits);
        std::printf("%d-bit mask:", bits);
        for (int l = 27; l <= 45; ++l) {
            if (pred[static_cast<std::size_t>(l)]) {
                std::printf(" %d:%d", l, s[static_cast<std::size_t>(l)]);
            }
        }
        std::printf("  ...\n");
    }
    return 0;
}
