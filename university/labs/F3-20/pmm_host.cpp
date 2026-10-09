// pmm_host.cpp - F3-20: host unit tests for memmap.h, bitmap_pmm.h and buddy.h (milestone B3,
// acceptance test 1): random allocate/free sequences against a reference model, no double
// allocation, all frames returned at the end, contiguous and aligned requests honoured.
#include <cstdint>
#include <cstdio>
#include <random>
#include <vector>
#include "bitmap_pmm.h"
#include "buddy.h"
#include "memmap.h"

int g_fail = 0;
#define CHECK(cond)                                                    \
    do {                                                               \
        if (!(cond)) {                                                 \
            std::printf("FAIL line %d: %s\n", __LINE__, #cond);        \
            ++g_fail;                                                  \
        }                                                              \
    } while (0)

void test_memmap()
{
    memmap::RangeList l;
    l.r[l.n++] = {0x100000, 0x7fe0000};        // like QEMU's 128 MiB map, out of order
    l.r[l.n++] = {0x0, 0x9fc00};               // ends mid-page: rounded down to 0x9f000
    l.r[l.n++] = {0x7fd0000, 0x8000000};       // overlaps the first range
    l.r[l.n++] = {0x500, 0x800};               // smaller than a page after rounding
    memmap::normalize(l);
    CHECK(l.n == 2);
    CHECK(l.r[0].base == 0x0 && l.r[0].end == 0x9f000);
    CHECK(l.r[1].base == 0x100000 && l.r[1].end == 0x8000000);
    memmap::subtract(l, 0x0, 0x100000);       // keep all of low memory
    memmap::subtract(l, 0x100000, 0x114321);  // kernel image, end rounded up to 0x115000
    CHECK(l.n == 1 && l.r[0].base == 0x115000 && l.r[0].end == 0x8000000);
    memmap::subtract(l, 0x200000, 0x201000);  // a hole in the middle splits the range
    CHECK(l.n == 2 && l.r[0].end == 0x200000 && l.r[1].base == 0x201000);
    std::printf("memmap: normalize and subtract: %s\n", g_fail == 0 ? "ok" : "FAILED");
}

// Reference model: who owns each frame (0 = free, otherwise an allocation id).
struct Model {
    std::vector<uint32_t> owner;
    struct Block {
        uint64_t frame;
        int order;
        uint32_t id;
    };
    std::vector<Block> live;
};

void test_buddy(uint64_t ops, uint32_t seed)
{
    const uint64_t nframes = 16384;            // a 64 MiB "machine"
    std::vector<uint8_t> ram(nframes * 4096);
    std::vector<uint8_t> state(nframes);
    Buddy b;
    b.init(ram.data(), state.data(), nframes, true);
    std::vector<bool> usable(nframes, false);
    auto add = [&](uint64_t first, uint64_t count) {
        b.add_frames(first, count);
        for (uint64_t f = first; f < first + count; ++f) {
            usable[f] = true;
        }
    };
    add(277, 3000);                            // odd boundaries on purpose
    add(4100, nframes - 4100);
    const uint64_t initial = b.stats().free_frames;
    CHECK(initial == 3000 + nframes - 4100);
    Buddy::Stats start = b.stats();
    {   // exhaust memory one frame at a time, then give everything back
        std::vector<uint64_t> all;
        uint64_t f;
        while (b.alloc(0, f)) {
            all.push_back(f);
        }
        CHECK(all.size() == initial && b.stats().free_frames == 0);
        for (uint64_t x : all) {
            CHECK(b.free(x, 0));
        }
        CHECK(b.stats().free_frames == initial);
        std::printf("buddy: allocated all %zu frames one by one, freed them, free count back to %llu\n",
                    all.size(), static_cast<unsigned long long>(b.stats().free_frames));
    }

    Model m;
    m.owner.assign(nframes, 0);
    std::mt19937 rng(seed);
    uint32_t next_id = 1;
    uint64_t failed_allocs = 0;
    uint64_t max_live = 0;
    for (uint64_t i = 0; i < ops; ++i) {
        bool do_alloc = m.live.empty() || rng() % 100 < 50;
        if (do_alloc) {
            int order = static_cast<int>(rng() % 100 < 70 ? 0 : rng() % 6);
            uint64_t f;
            if (!b.alloc(order, f)) {
                ++failed_allocs;
                continue;
            }
            CHECK(f % (1ull << order) == 0);   // aligned to its own size
            for (uint64_t k = f; k < f + (1ull << order); ++k) {
                if (m.owner[k] != 0 || !usable[k]) {
                    std::printf("FAIL: frame %llu handed out twice or not usable\n",
                                static_cast<unsigned long long>(k));
                    ++g_fail;
                    return;
                }
                m.owner[k] = next_id;
            }
            m.live.push_back({f, order, next_id++});
        } else {
            size_t idx = rng() % m.live.size();
            Model::Block blk = m.live[idx];
            CHECK(b.free(blk.frame, blk.order));
            for (uint64_t k = blk.frame; k < blk.frame + (1ull << blk.order); ++k) {
                m.owner[k] = 0;
            }
            m.live[idx] = m.live.back();
            m.live.pop_back();
        }
        if (m.live.size() > max_live) {
            max_live = m.live.size();
        }
    }
    for (const auto& blk : m.live) {
        CHECK(b.free(blk.frame, blk.order));
    }
    // error detection: double free and wrong order are refused
    uint64_t f;
    CHECK(b.alloc(2, f));
    CHECK(!b.free(f, 1));                      // wrong order
    CHECK(b.free(f, 2));
    CHECK(!b.free(f, 2));                      // double free
    const Buddy::Stats& end = b.stats();
    CHECK(end.free_frames == initial);
    bool same_shape = true;
    for (int o = 0; o <= Buddy::kMaxOrder; ++o) {
        same_shape = same_shape && end.free_blocks[o] == start.free_blocks[o];
    }
    CHECK(same_shape);                         // everything merged back to the start state
    std::printf("buddy: %llu random operations (seed %u), peak %llu live blocks, %llu allocs "
                "refused when full; free frames %llu -> %llu, block shape restored: %s\n",
                static_cast<unsigned long long>(ops), seed, static_cast<unsigned long long>(max_live),
                static_cast<unsigned long long>(failed_allocs),
                static_cast<unsigned long long>(initial),
                static_cast<unsigned long long>(end.free_frames), same_shape ? "yes" : "NO");
}

void test_bitmap()
{
    const uint64_t nframes = 4096;
    std::vector<uint64_t> bits(nframes / 64);
    BitmapPmm p;
    p.init(bits.data(), nframes);
    p.mark_free(256, nframes - 256);
    uint64_t start = p.free_frames();
    std::mt19937 rng(7);
    std::vector<std::pair<uint64_t, uint64_t>> live;
    std::vector<bool> used(nframes, false);
    for (int i = 0; i < 20000; ++i) {
        if (live.empty() || rng() % 2 == 0) {
            uint64_t count = 1 + rng() % 8;
            uint64_t align = (rng() % 4 == 0) ? 16 : 1;
            uint64_t f;
            if (p.alloc(count, align, f)) {
                CHECK(f % align == 0 && f >= 256);
                for (uint64_t k = f; k < f + count; ++k) {
                    CHECK(!used[k]);
                    used[k] = true;
                }
                live.push_back({f, count});
            }
        } else {
            size_t idx = rng() % live.size();
            CHECK(p.free(live[idx].first, live[idx].second));
            for (uint64_t k = live[idx].first; k < live[idx].first + live[idx].second; ++k) {
                used[k] = false;
            }
            live[idx] = live.back();
            live.pop_back();
        }
    }
    for (auto& [f, c] : live) {
        CHECK(p.free(f, c));
    }
    CHECK(!p.free(300, 1));                    // double free detected
    CHECK(p.free_frames() == start);
    std::printf("bitmap: 20000 random operations, free frames %llu -> %llu\n",
                static_cast<unsigned long long>(start), static_cast<unsigned long long>(p.free_frames()));
}

void test_leak_without_order_check()
{
    // The forensic lab's bug in miniature: an order-1 block freed as order 0.
    const uint64_t nframes = 64;
    std::vector<uint8_t> ram(nframes * 4096), state(nframes);
    Buddy b;
    b.init(ram.data(), state.data(), nframes, false);
    b.add_frames(0, nframes);
    uint64_t before = b.stats().free_frames;
    uint64_t f;
    CHECK(b.alloc(1, f));
    CHECK(b.free(f, 0));                       // accepted: without order checking, no error
    std::printf("leak demo (order check off): free frames %llu -> %llu after alloc(order 1) + "
                "free(order 0)\n", static_cast<unsigned long long>(before),
                static_cast<unsigned long long>(b.stats().free_frames));
    CHECK(b.stats().free_frames == before - 1);
}

int main()
{
    test_memmap();
    test_buddy(200000, 1);
    test_buddy(200000, 2);
    test_bitmap();
    test_leak_without_order_check();
    std::printf("%s: %d failure(s)\n", g_fail == 0 ? "PASS" : "FAIL", g_fail);
    return g_fail == 0 ? 0 : 1;
}
