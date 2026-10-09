// heap_host.cpp - F3-22: the kernel's heap.h and containers.h, tested on the host under
// AddressSanitizer and UndefinedBehaviorSanitizer (milestone B5, acceptance test 3).
// The "environment" gives the heap memory from std::aligned_alloc instead of the PMM.
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <set>
#include <string>
#include <vector>
#include "containers.h"
#include "heap.h"

namespace {

std::set<const void*> g_large;
long g_slabs_out = 0, g_pages_out = 0;
std::string g_last_report;

void* get_slab()
{
    ++g_slabs_out;
    return std::aligned_alloc(Heap::kSlabSize, Heap::kSlabSize);
}
void put_slab(void* p)
{
    --g_slabs_out;
    std::free(p);
}
void* get_pages(size_t pages)
{
    void* p = std::aligned_alloc(Heap::kPage, pages * Heap::kPage);
    g_large.insert(static_cast<uint8_t*>(p) + Heap::kPage);
    g_pages_out += static_cast<long>(pages);
    return p;
}
void put_pages(void* p, size_t pages)
{
    g_large.erase(static_cast<uint8_t*>(p) + Heap::kPage);
    g_pages_out -= static_cast<long>(pages);
    std::free(p);
}
bool is_large(const void* p)
{
    return g_large.count(p) != 0;
}
void report(const char* what, const char* cache, const void*, size_t offset, uint8_t value)
{
    char buf[160];
    std::snprintf(buf, sizeof buf, "%s in %s at byte %zu (0x%02x)", what, cache, offset, unsigned{value});
    g_last_report = buf;
}

int g_fail = 0;
#define CHECK(c)                                                         \
    do {                                                                 \
        if (!(c)) {                                                      \
            std::printf("FAIL line %d: %s\n", __LINE__, #c);             \
            ++g_fail;                                                    \
        }                                                                \
    } while (0)

} // namespace

int main()
{
    Heap h;
    h.init(HeapEnv{get_slab, put_slab, get_pages, put_pages, is_large, report});

    // 1. the stress test: one million allocations of mixed sizes and alignments
    struct Live {
        uint8_t* p = nullptr;
        size_t size = 0;
        uint8_t tag = 0;
    };
    std::vector<Live> live(4096);
    std::mt19937_64 rng(42);
    long allocs = 0, corrupt = 0;
    while (allocs < 1000000) {
        Live& s = live[rng() % live.size()];
        if (s.p != nullptr) {
            for (size_t i = 0; i < s.size; i += 61) {           // check a sample of the bytes
                corrupt += s.p[i] != s.tag;
            }
            h.free(s.p);
            s.p = nullptr;
            continue;
        }
        uint64_t r = rng();
        s.size = (r % 1000 == 0) ? 4096 + r % 60000 : 2 + r % 2047;
        size_t align = size_t{1} << (r >> 20) % 9;
        s.p = static_cast<uint8_t*>(h.alloc(s.size, align));
        CHECK(s.p != nullptr && reinterpret_cast<uintptr_t>(s.p) % align == 0);
        s.tag = static_cast<uint8_t>(r >> 32);
        for (size_t i = 0; i < s.size; ++i) {                    // ASan checks every byte is ours
            s.p[i] = s.tag;
        }
        ++allocs;
    }
    for (Live& s : live) {
        h.free(s.p);
        s.p = nullptr;
    }
    h.trim();
    std::printf("stress: %ld allocations, corrupted bytes %ld, live objects %llu, slabs still out %ld, "
                "large pages still out %ld\n", allocs, corrupt, static_cast<unsigned long long>(h.live_objects()),
                g_slabs_out, g_pages_out);
    CHECK(corrupt == 0 && h.live_objects() == 0 && g_slabs_out == 0 && g_pages_out == 0);

    // 2. a write after free is reported, naming the cache
    char* p = static_cast<char*>(h.alloc(100));
    h.free(p);
    p[50] = 'X';                                    // deliberate: the memory is still the heap's
    void* q = h.alloc(100);
    std::printf("use-after-free: report \"%s\"\n", g_last_report.c_str());
    CHECK(g_last_report.find("kmalloc-128") != std::string::npos);
    h.free(q);

    // 3. a double free is reported
    g_last_report.clear();
    void* d = h.alloc(24);
    h.free(d);
    h.free(d);
    std::printf("double free: report \"%s\"\n", g_last_report.c_str());
    CHECK(g_last_report.find("double free") != std::string::npos);

    // 4. the containers on the host
    KVector<std::string> words;
    for (int i = 0; i < 1000; ++i) {
        CHECK(words.push_back(std::to_string(i)));
    }
    KHashMap<int> map;
    for (int i = 0; i < 1000; ++i) {
        CHECK(map.put(static_cast<uint64_t>(i) * 31, i));
    }
    std::printf("containers: %zu strings (last \"%s\"), map size %zu, map[31*999] = %d\n", words.size(),
                words[999].c_str(), map.size(), *map.get(31 * 999));
    std::printf("%s: %d failure(s)\n", g_fail == 0 ? "PASS" : "FAIL", g_fail);
    h.trim();
    return g_fail == 0 ? 0 : 1;
}
