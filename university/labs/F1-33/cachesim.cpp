// Cache simulator driver. Input: first line "sets ways lineBytes", then one hexadecimal
// address per line (a trace). Output: one line per access, then the totals.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

#include "cache.hpp"

int main()
{
    unsigned sets = 0;
    unsigned ways = 0;
    unsigned lineBytes = 0;
    if (!(std::cin >> sets >> ways >> lineBytes)) {
        std::cerr << "expected: sets ways lineBytes\n";
        return 1;
    }
    Cache cache(sets, ways, lineBytes);
    std::printf("cache: %u sets x %u ways x %u-byte lines = %u bytes\n", sets, ways, lineBytes,
                sets * ways * lineBytes);
    std::printf("%4s %8s %4s %6s %-5s %s\n", "#", "address", "set", "tag", "", "evicted tag");
    std::string word;
    int n = 0;
    while (std::cin >> word) {
        const std::uint64_t address = std::stoull(word, nullptr, 16);
        const AccessResult r = cache.access(address);
        std::printf("%4d %8llx %4llu %6llx %-5s", ++n, static_cast<unsigned long long>(address),
                    static_cast<unsigned long long>(r.set), static_cast<unsigned long long>(r.tag),
                    r.hit ? "hit" : "MISS");
        if (r.evicted) {
            std::printf(" %llx", static_cast<unsigned long long>(r.evictedTag));
        }
        std::printf("\n");
    }
    std::printf("hits %llu, misses %llu\n", static_cast<unsigned long long>(cache.hits()),
                static_cast<unsigned long long>(cache.misses()));
    return 0;
}
