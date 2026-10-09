// F1-45 Listing 2: sizing a BAR. Software writes all ones to the BAR, reads it
// back, masks off the low flag bits, inverts and adds one. The read-back values
// in bar_size.in are the ones this rule predicts for the BAR sizes QEMU printed
// (see qemu_info_pci.out); they were not read from a device.
#include <linux/pci_regs.h>

#include <cstdint>
#include <cstdio>
#include <iostream>
#include <string>

std::uint64_t bar_size(const std::string& kind, std::uint32_t lo, std::uint32_t hi)
{
    if (kind == "io") {
        const std::uint32_t mask = static_cast<std::uint32_t>(PCI_BASE_ADDRESS_IO_MASK);
        return static_cast<std::uint32_t>(~(lo & mask) + 1u);
    }
    const std::uint32_t mask = static_cast<std::uint32_t>(PCI_BASE_ADDRESS_MEM_MASK);
    std::uint64_t v = lo & mask;
    if (kind == "mem64") {
        v |= static_cast<std::uint64_t>(hi) << 32;   // two BAR slots form one address
        return ~v + 1u;
    }
    return static_cast<std::uint32_t>(~static_cast<std::uint32_t>(v) + 1u);
}

int main()
{
    std::string name, kind;
    while (std::cin >> name >> kind) {
        std::string lo_s, hi_s = "0";
        std::cin >> lo_s;
        if (kind == "mem64") {
            std::cin >> hi_s;
        }
        const auto lo = static_cast<std::uint32_t>(std::stoul(lo_s, nullptr, 16));
        const auto hi = static_cast<std::uint32_t>(std::stoul(hi_s, nullptr, 16));
        const unsigned flags = lo & (kind == "io" ? 0x3u : 0xFu);
        const std::uint64_t size = bar_size(kind, lo, hi);
        std::printf("%-11s %-5s readback 0x%08X  flag bits 0x%X", name.c_str(), kind.c_str(), lo,
                    flags);
        if (kind != "io") {
            std::printf(" (%s, %s)", (lo & PCI_BASE_ADDRESS_MEM_TYPE_64) ? "64-bit" : "32-bit",
                        (lo & PCI_BASE_ADDRESS_MEM_PREFETCH) ? "prefetchable" : "non-prefetchable");
        }
        std::printf("  size 0x%llX = %llu bytes\n", static_cast<unsigned long long>(size),
                    static_cast<unsigned long long>(size));
    }
    return 0;
}
