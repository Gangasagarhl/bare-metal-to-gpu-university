// F1-45 Listing 1: read the PCI configuration space of every PCI function of
// the machine this program runs on (Linux exposes it as a file per device under
// /sys/bus/pci/devices) and decode the standard header fields and the
// capability list. Offsets and IDs come from the Linux UAPI header pci_regs.h.
#include <linux/pci_regs.h>

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

std::uint16_t le16(const std::vector<std::uint8_t>& c, std::size_t off)
{
    return static_cast<std::uint16_t>(c[off] | (c[off + 1] << 8));   // little-endian
}

const char* cap_name(int id)
{
    switch (id) {
    case PCI_CAP_ID_PM: return "power management";
    case PCI_CAP_ID_MSI: return "MSI";
    case PCI_CAP_ID_VNDR: return "vendor-specific";
    case PCI_CAP_ID_EXP: return "PCI Express";
    case PCI_CAP_ID_MSIX: return "MSI-X";
    default: return "other";
    }
}

int main()
{
    const fs::path root{"/sys/bus/pci/devices"};
    if (!fs::exists(root)) {
        std::printf("no %s on this machine\n", root.c_str());
        return 0;
    }
    std::vector<fs::path> devs;
    for (const auto& e : fs::directory_iterator(root)) {
        devs.push_back(e.path());
    }
    std::sort(devs.begin(), devs.end());
    for (const fs::path& d : devs) {
        std::ifstream f(d / "config", std::ios::binary);
        std::vector<std::uint8_t> c((std::istreambuf_iterator<char>(f)),
                                    std::istreambuf_iterator<char>());
        if (c.size() < PCI_STD_HEADER_SIZEOF) {
            std::printf("%s: could not read the header\n", d.filename().c_str());
            continue;
        }
        std::printf("%s  %04x:%04x  class %02x/%02x/%02x  header type 0x%02x  cmd 0x%04x",
                    d.filename().c_str(), le16(c, PCI_VENDOR_ID), le16(c, PCI_DEVICE_ID),
                    c[PCI_CLASS_DEVICE + 1], c[PCI_CLASS_DEVICE], c[PCI_CLASS_PROG],
                    c[PCI_HEADER_TYPE], le16(c, PCI_COMMAND));
        if ((le16(c, PCI_STATUS) & PCI_STATUS_CAP_LIST) == 0) {
            std::printf("  (no capability list)\n");
            continue;
        }
        std::printf("\n    capabilities:");
        std::size_t p = c[PCI_CAPABILITY_LIST] & ~3u;
        for (int guard = 0; p != 0 && p + 1 < c.size() && guard < 48; ++guard) {
            std::printf(" [0x%02zx] %s (0x%02x)", p, cap_name(c[p]), c[p]);
            p = c[p + 1] & ~3u;           // next pointer
        }
        if (p != 0) {
            std::printf(" ... (beyond the %zu bytes readable here)", c.size());
        }
        std::printf("\n");
    }
    return 0;
}
