// pagesize_trap.cpp - BR-06: the same three page computations done two ways:
//   "hard-coded": with a constant 4096, as code written only for a PC often is;
//   "from arch":  with the page size the arch layer reports (here: a parameter).
// Run for the three page sizes an AArch64 kernel may choose (4, 16 and 64 KiB).
#include <cstdint>
#include <cstdio>
#include <initializer_list>

namespace {

constexpr std::uint64_t kHardCodedPage = 4096;   // the trap

struct PageMath {
    std::uint64_t frames;        // how many page frames 256 MiB of RAM holds
    std::uint64_t page_base;     // base of the page that contains an address
    bool aligned;                // may a mapping start at this address?
};

PageMath compute(std::uint64_t page, std::uint64_t ram, std::uint64_t addr, std::uint64_t map_at)
{
    return {ram / page, addr & ~(page - 1), (map_at & (page - 1)) == 0};
}

} // namespace

int main()
{
    const std::uint64_t ram = 256ull << 20;
    const std::uint64_t addr = 0x40127456;   // some kernel address in RAM
    const std::uint64_t map_at = 0x40127000; // a mapping request computed elsewhere
    std::printf("RAM 256 MiB, address 0x%llx, mapping request at 0x%llx\n",
                static_cast<unsigned long long>(addr), static_cast<unsigned long long>(map_at));
    std::printf("%-10s %-11s %8s %12s %9s\n", "real page", "computed by", "frames", "page base", "aligned?");
    int wrong = 0;
    for (std::uint64_t real : {4096ull, 16384ull, 65536ull}) {
        const PageMath good = compute(real, ram, addr, map_at);
        const PageMath bad = compute(kHardCodedPage, ram, addr, map_at);
        const bool differs = good.frames != bad.frames || good.page_base != bad.page_base || good.aligned != bad.aligned;
        wrong += differs ? 1 : 0;
        std::printf("%-10llu %-11s %8llu   0x%08llx %9s\n", static_cast<unsigned long long>(real), "arch layer",
                    static_cast<unsigned long long>(good.frames), static_cast<unsigned long long>(good.page_base),
                    good.aligned ? "yes" : "NO");
        std::printf("%-10s %-11s %8llu   0x%08llx %9s%s\n", "", "hard-coded", static_cast<unsigned long long>(bad.frames),
                    static_cast<unsigned long long>(bad.page_base), bad.aligned ? "yes" : "NO",
                    differs ? "   <- wrong" : "");
    }
    std::printf("page sizes where the hard-coded constant gives wrong answers: %d of 3\n", wrong);
    return 0;
}
