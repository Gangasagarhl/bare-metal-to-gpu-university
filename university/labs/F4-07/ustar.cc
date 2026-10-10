// ustar.cc - DR301 F4-07 (project): read-only USTAR root.
#include "ustar.h"
#include "kbase.h"

namespace ustar {
namespace {
constexpr uint32_t WINDOW = 64 * 1024;
alignas(4096) uint8_t g_win[WINDOW];
uint64_t g_win_off = ~uint64_t{0};

// One byte range through a 64 KiB window aligned to 64 KiB.
const uint8_t* at(ReadFn read, uint64_t off)
{
    const uint64_t base = off & ~uint64_t{WINDOW - 1};
    if (base != g_win_off) {
        if (read(base, g_win, WINDOW) != 0) return nullptr;
        g_win_off = base;
    }
    return g_win + (off - base);
}

uint64_t octal(const uint8_t* p, int n)
{
    uint64_t v = 0;
    for (int i = 0; i < n && p[i] >= '0' && p[i] <= '7'; ++i) v = v * 8 + (p[i] - '0');
    return v;
}
}  // namespace

int walk(ReadFn read, void (*fn)(const Entry& e))
{
    int count = 0;
    for (uint64_t off = 0;;) {
        const uint8_t* h = at(read, off);                 // a header never crosses 64 KiB: 512-aligned
        if (!h) return -2;
        if (h[0] == 0) return count;                      // the first of the two zero blocks
        uint32_t sum = 0;                                 // checksum: the field itself counts as spaces
        for (int i = 0; i < 512; ++i) sum += (i >= 148 && i < 156) ? ' ' : h[i];
        if (sum != octal(h + 148, 8) || memcmp(h + 257, "ustar", 5) != 0) return -1;
        Entry e{};
        size_t n = 0;
        if (h[345]) {                                     // prefix "/" name
            for (int i = 0; i < 155 && h[345 + i]; ++i) e.path[n++] = static_cast<char>(h[345 + i]);
            e.path[n++] = '/';
        }
        for (int i = 0; i < 100 && h[i]; ++i) e.path[n++] = static_cast<char>(h[i]);
        e.path[n] = 0;
        e.type = static_cast<char>(h[156]);
        e.size = octal(h + 124, 12);
        e.data_off = off + 512;
        fn(e);
        ++count;
        off += 512 + ((e.size + 511) & ~uint64_t{511});
    }
}

int hash(ReadFn read, const Entry& e, uint32_t& out)
{
    uint32_t h = 2166136261u;
    for (uint64_t done = 0; done < e.size;) {
        const uint8_t* p = at(read, e.data_off + done);
        if (!p) return -2;
        const uint64_t in_win = WINDOW - ((e.data_off + done) & (WINDOW - 1));
        const uint64_t n = (e.size - done < in_win) ? e.size - done : in_win;
        h = fnv1a(p, static_cast<size_t>(n), h);
        done += n;
    }
    out = h;
    return 0;
}

int read_file(ReadFn read, const Entry& e, char* dst, uint32_t max)
{
    const uint32_t n = static_cast<uint32_t>(e.size < max ? e.size : max);
    for (uint32_t i = 0; i < n; ++i) {
        const uint8_t* p = at(read, e.data_off + i);
        if (!p) return -2;
        dst[i] = static_cast<char>(*p);
    }
    return static_cast<int>(n);
}
}  // namespace ustar
