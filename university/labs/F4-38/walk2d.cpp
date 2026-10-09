// walk2d.cpp - count the memory reads of a two-dimensional (nested) page walk.
// A guest page table maps guest-virtual to guest-physical addresses; a nested table maps
// guest-physical to host-physical. Every guest table entry lives at a guest-physical
// address, so reading it needs a nested walk first. Real tables, real walks, counted.
#include <cstdint>
#include <cstdio>
#include <map>

namespace {
constexpr std::uint64_t kPage = 4096;

// A radix page table with `levels` levels of 512 entries over a simulated memory.
// Table pages are allocated in the given address space; reads are counted by the caller.
struct Memory {
    std::map<std::uint64_t, std::uint64_t> words;   // address -> 64-bit entry
    std::uint64_t next_free;
    long reads = 0;
    std::uint64_t read(std::uint64_t addr) { ++reads; return words[addr]; }
};

struct Table {
    int levels;
    std::uint64_t root;
    Memory* mem;
    // Map one page: create the missing tables on the way down (not counted as walk reads).
    void map(std::uint64_t va, std::uint64_t pa)
    {
        std::uint64_t t = root;
        for (int l = levels - 1; l > 0; --l) {
            std::uint64_t slot = t + ((va >> (12 + 9 * l)) & 511) * 8;
            if (mem->words[slot] == 0) {
                mem->words[slot] = mem->next_free;
                mem->next_free += kPage;
            }
            t = mem->words[slot];
        }
        mem->words[t + ((va >> 12) & 511) * 8] = pa & ~(kPage - 1);
    }
};

// One-dimensional walk: `levels` reads.
std::uint64_t walk(const Table& t, std::uint64_t va, Memory& m)
{
    std::uint64_t a = t.root;
    for (int l = t.levels - 1; l >= 0; --l) {
        a = m.read(a + ((va >> (12 + 9 * l)) & 511) * 8);
    }
    return a | (va & (kPage - 1));
}

// Two-dimensional walk: every guest table address (a guest-physical address) and the
// final guest-physical address are translated by the nested table first.
std::uint64_t walk2d(const Table& guest, const Table& nested, std::uint64_t gva, Memory& host)
{
    std::uint64_t gpa_table = guest.root;
    for (int l = guest.levels - 1; l >= 0; --l) {
        std::uint64_t entry_gpa = gpa_table + ((gva >> (12 + 9 * l)) & 511) * 8;
        std::uint64_t entry_hpa = walk(nested, entry_gpa, host);   // nested walk
        gpa_table = host.read(entry_hpa);                         // the guest entry itself
    }
    std::uint64_t data_gpa = gpa_table | (gva & (kPage - 1));
    return walk(nested, data_gpa, host);                          // translate the data address
}

void experiment(int n, int m)
{
    Memory host{{}, 0x100000};
    Table nested{m, host.next_free, &host};
    host.next_free += kPage;
    // Guest-physical memory: 256 pages at gpa 0, at host 0x40000000; the guest's tables
    // live in it too.
    for (std::uint64_t gpa = 0; gpa < 256 * kPage; gpa += kPage) {
        nested.map(gpa, 0x40000000 + gpa);
    }
    // The guest's own table, stored in guest-physical memory (we keep its words in the
    // host memory map at their host-physical addresses, as the hardware would see them).
    Memory guest_alloc{{}, kPage};
    Table guest{n, 0, &guest_alloc};
    const std::uint64_t gva = 0x7f1234567000ull & ((1ull << (12 + 9 * n)) - 1);
    guest.map(gva, 0x80000);                          // the data page is gpa 0x80000
    for (const auto& [gpa, val] : guest_alloc.words) {
        host.words[walk(nested, gpa, host)] = val;    // place the guest tables in host RAM
    }
    host.reads = 0;
    std::uint64_t hpa = walk2d(guest, nested, gva + 0x123, host);
    std::printf("guest %d levels, nested %d levels: %3ld reads (formula (n+1)(m+1)-1 = %2d), "
                "native walk %d reads; gva %#llx -> hpa %#llx\n",
                n, m, host.reads, (n + 1) * (m + 1) - 1, n,
                static_cast<unsigned long long>(gva + 0x123), static_cast<unsigned long long>(hpa));
}
}

int main()
{
    experiment(2, 2);
    experiment(3, 3);
    experiment(4, 4);
    experiment(4, 3);   // one nested level fewer, as 2 MiB nested pages give
    experiment(5, 5);
    return 0;
}
