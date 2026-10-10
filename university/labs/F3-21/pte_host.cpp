// pte_host.cpp - F3-21: host test of the address split and of reading a page-table entry.
// The two virtual addresses are the lab's walk target and the start of the direct map;
// the entry value is the PT entry QEMU's monitor printed in the lab (walk.out).
#include <cstdint>
#include <cstdio>
#include "paging.h"

int main()
{
    int fail = 0;
    struct Case {
        uint64_t virt;
        unsigned pml4, pdpt, pd, pt;
        uint64_t offset;
    } cases[] = {
        {0xffffffff80114008ull, 511, 510, 0, 276, 0x008},   // kernel .data (walk target)
        {0xffff800000000000ull, 256, 0, 0, 0, 0x000},       // first byte of the direct map
        {0xffffff0000001000ull, 510, 0, 0, 1, 0x000},       // first page of the kernel stack
        {0x0000000000401abcull, 0, 0, 2, 1, 0xabc},         // a low (user-half) address
    };
    for (const Case& c : cases) {
        paging::Split s = paging::split(c.virt);
        bool ok = s.pml4 == c.pml4 && s.pdpt == c.pdpt && s.pd == c.pd && s.pt == c.pt && s.offset == c.offset;
        std::printf("%-4s %016llx -> PML4 %3u  PDPT %3u  PD %3u  PT %3u  offset 0x%03llx\n", ok ? "ok" : "FAIL",
                    static_cast<unsigned long long>(c.virt), s.pml4, s.pdpt, s.pd, s.pt,
                    static_cast<unsigned long long>(s.offset));
        fail += ok ? 0 : 1;
    }
    const uint64_t e = 0x8000000000114163ull;
    std::printf("entry %016llx: frame 0x%llx;%s%s%s%s%s%s%s%s\n", static_cast<unsigned long long>(e),
                static_cast<unsigned long long>(e & paging::kAddrMask),
                (e & paging::kPresent) ? " present" : "", (e & paging::kWrite) ? " writable" : " read-only",
                (e & paging::kUser) ? " user" : " supervisor-only", (e & paging::kAccessed) ? " accessed" : "",
                (e & paging::kDirty) ? " dirty" : "", (e & paging::kGlobal) ? " global" : "",
                (e & paging::kNoExec) ? " no-execute" : " executable", (e & paging::kCacheDisable) ? " uncached" : "");
    fail += (e & paging::kAddrMask) == 0x114000 ? 0 : 1;
    std::printf("%s: %d failure(s)\n", fail == 0 ? "PASS" : "FAIL", fail);
    return fail == 0 ? 0 : 1;
}
