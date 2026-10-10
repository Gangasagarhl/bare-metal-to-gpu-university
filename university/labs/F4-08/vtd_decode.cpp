// vtd_decode.cpp - DR302 F4-08 worked example: builds MSI messages, splits IOVAs into the
// three page-table indices of a 39-bit VT-d domain, and decodes a raw fault record.
// Input (vtd_decode.in): lines "msi <vector> <apic id>", "iova <hex>", "fault <hi> <lo>".
// Field positions are the ones the kernel uses (msi.cc, vtd.cc), pending verification.
#include <cstdint>
#include <cstdio>
#include <iostream>
#include <sstream>
#include <string>

int main()
{
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream in(line);
        std::string what;
        in >> what;
        if (what == "msi") {
            unsigned vector = 0, apic = 0;
            in >> vector >> apic;
            const std::uint32_t addr = 0xFEE00000u | (apic << 12);
            const std::uint32_t data = vector & 0xFF;     // fixed delivery, edge trigger
            std::printf("MSI for vector %u (0x%02x) to APIC %u: address 0x%08x data 0x%04x\n", vector,
                        vector, apic, addr, data);
        } else if (what == "iova") {
            std::uint64_t iova = 0;
            in >> std::hex >> iova;
            std::printf("IOVA 0x%llx: L3 index %llu (bits 38:30), L2 index %llu (bits 29:21), "
                        "L1 index %llu (bits 20:12), offset 0x%llx\n",
                        static_cast<unsigned long long>(iova),
                        static_cast<unsigned long long>((iova >> 30) & 0x1FF),
                        static_cast<unsigned long long>((iova >> 21) & 0x1FF),
                        static_cast<unsigned long long>((iova >> 12) & 0x1FF),
                        static_cast<unsigned long long>(iova & 0xFFF));
        } else if (what == "fault") {
            std::uint64_t hi = 0, lo = 0;
            in >> std::hex >> hi >> lo;
            const unsigned sid = hi & 0xFFFF;
            std::printf("fault record hi 0x%016llx lo 0x%016llx\n", static_cast<unsigned long long>(hi),
                        static_cast<unsigned long long>(lo));
            std::printf("  F (bit 127) = %llu, T (bit 126) = %llu (%s)\n",
                        static_cast<unsigned long long>(hi >> 63),
                        static_cast<unsigned long long>((hi >> 62) & 1), ((hi >> 62) & 1) ? "read" : "write");
            std::printf("  fault reason (bits 103:96) = %llu\n", static_cast<unsigned long long>((hi >> 32) & 0xFF));
            std::printf("  source id (bits 79:64) = 0x%04x = bus %u, device %u, function %u\n", sid, sid >> 8,
                        (sid >> 3) & 0x1F, sid & 7);
            std::printf("  fault info (bits 63:12) = page 0x%llx\n", static_cast<unsigned long long>(lo & ~0xFFFull));
        }
    }
    return 0;
}
