// F1-45 forensic evidence: the BAR sizing routine of a learner's enumerator.
// Same input as Listing 2. Compare its sizes with QEMU's own listing.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

int main()
{
    std::string name, kind, lo_s, hi_s;
    while (std::cin >> name >> kind >> lo_s) {
        if (kind == "mem64") {
            std::cin >> hi_s;           // read but not used
        }
        const auto lo = static_cast<std::uint32_t>(std::stoul(lo_s, nullptr, 16));
        const std::uint32_t size = ~lo + 1u;
        std::printf("enumerator: %-11s size 0x%X (%u bytes)\n", name.c_str(),
                    static_cast<unsigned>(size), static_cast<unsigned>(size));
    }
    return 0;
}
