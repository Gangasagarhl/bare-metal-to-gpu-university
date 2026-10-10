// Address translation with a TLB in front of a page table (a model, 4 KiB pages).
// The page table is a map from virtual page number (VPN) to physical frame number (PFN);
// the TLB is fully associative with LRU replacement. Input: first line "tlbEntries", then
// one hexadecimal virtual address per line. Unmapped pages report a page fault.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <map>
#include <string>
#include <vector>

struct TlbEntry
{
    std::uint64_t vpn = 0;
    std::uint64_t pfn = 0;
    std::uint64_t lastUse = 0;
};

int main()
{
    const std::map<std::uint64_t, std::uint64_t> pageTable = {
        {0x0, 0x7}, {0x1, 0x3}, {0x2, 0x9}, {0x5, 0x1}, {0x40, 0x2}};
    std::size_t tlbEntries = 0;
    std::cin >> tlbEntries;
    std::vector<TlbEntry> tlb;
    std::uint64_t clock = 0;
    int hits = 0;
    int walks = 0;
    std::string word;
    while (std::cin >> word) {
        const std::uint64_t va = std::stoull(word, nullptr, 16);
        const std::uint64_t vpn = va >> 12;
        const std::uint64_t offset = va & 0xFFF;
        ++clock;
        TlbEntry* found = nullptr;
        for (TlbEntry& e : tlb) {
            if (e.vpn == vpn) {
                found = &e;
            }
        }
        const char* how = "TLB hit";
        if (found != nullptr) {
            ++hits;
        } else {
            ++walks;
            const auto pte = pageTable.find(vpn);
            if (pte == pageTable.end()) {
                std::printf("va 0x%05llx  vpn 0x%02llx  TLB miss, walk: not mapped -> PAGE FAULT\n",
                            static_cast<unsigned long long>(va),
                            static_cast<unsigned long long>(vpn));
                continue;
            }
            how = "TLB miss, walk";
            if (tlb.size() < tlbEntries) {
                tlb.push_back(TlbEntry{});
                found = &tlb.back();
            } else {                                          // replace the least recently used
                found = &tlb[0];
                for (TlbEntry& e : tlb) {
                    if (e.lastUse < found->lastUse) {
                        found = &e;
                    }
                }
            }
            found->vpn = vpn;
            found->pfn = pte->second;
        }
        found->lastUse = clock;
        const std::uint64_t pa = (found->pfn << 12) | offset;
        std::printf("va 0x%05llx  vpn 0x%02llx  %-15s -> pfn 0x%llx, pa 0x%05llx\n",
                    static_cast<unsigned long long>(va), static_cast<unsigned long long>(vpn), how,
                    static_cast<unsigned long long>(found->pfn),
                    static_cast<unsigned long long>(pa));
    }
    std::printf("TLB hits %d, page-table walks %d\n", hits, walks);
    return 0;
}
