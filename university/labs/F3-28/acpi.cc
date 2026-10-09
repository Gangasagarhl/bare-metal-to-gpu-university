// acpi.cc - find the MADT and list the processors it describes. Structure layouts as
// read by this code: ACPI Specification, "ACPI Software Programming Model" (RSDP,
// RSDT/XSDT, table header) and the MADT section (title only; see the unverified box).
#include "cpu.h"

namespace k {

namespace {
bool checksum_ok(const uint8_t* p, size_t n)
{
    uint8_t sum = 0;
    for (size_t i = 0; i < n; ++i) {
        sum = uint8_t(sum + p[i]);
    }
    return sum == 0;
}

const uint8_t* find_rsdp()
{
    uint64_t ebda = uint64_t(*reinterpret_cast<const uint16_t*>(0x40E)) << 4;
    const uint64_t ranges[2][2] = {{ebda, ebda + 1024}, {0xE0000, 0x100000}};
    for (auto& r : ranges) {
        for (uint64_t a = r[0]; a + 20 <= r[1]; a += 16) {
            auto* p = reinterpret_cast<const uint8_t*>(a);
            if (memcmp(p, "RSD PTR ", 8) == 0 && checksum_ok(p, 20)) {
                return p;
            }
        }
    }
    return nullptr;
}

template <typename T> T get(const uint8_t* p, size_t off)
{
    T v;
    memcpy(&v, p + off, sizeof(T));
    return v;
}
}  // namespace

// Fills apic_ids with the enabled processors of the MADT; returns how many.
int acpi_list_cpus(uint32_t* apic_ids, int max, bool verbose)
{
    const uint8_t* rsdp = find_rsdp();
    if (rsdp == nullptr) {
        kprintf("acpi: no RSDP found\n");
        return 0;
    }
    uint8_t rev = rsdp[15];
    bool xsdt = rev >= 2;
    auto* root = reinterpret_cast<const uint8_t*>(
        xsdt ? get<uint64_t>(rsdp, 24) : uint64_t(get<uint32_t>(rsdp, 16)));
    uint32_t root_len = get<uint32_t>(root, 4);
    if (verbose) kprintf("acpi: RSDP revision %u at %p, %s at %p, checksum %s\n", rev, rsdp,
            xsdt ? "XSDT" : "RSDT", root, checksum_ok(root, root_len) ? "ok" : "BAD");
    int entry_size = xsdt ? 8 : 4;
    for (uint32_t off = 36; off + entry_size <= root_len; off += entry_size) {
        auto* t = reinterpret_cast<const uint8_t*>(
            xsdt ? get<uint64_t>(root, off) : uint64_t(get<uint32_t>(root, off)));
        uint32_t len = get<uint32_t>(t, 4);
        if (verbose) kprintf("acpi: table %c%c%c%c at %p, %u bytes, checksum %s\n", t[0], t[1], t[2], t[3], t,
                len, checksum_ok(t, len) ? "ok" : "BAD");
        if (memcmp(t, "APIC", 4) != 0) {
            continue;
        }
        int n = 0;
        if (verbose) kprintf("madt: local APIC address %x, flags %x\n", get<uint32_t>(t, 36),
                get<uint32_t>(t, 40));
        for (uint32_t e = 44; e + 2 <= len; e += t[e + 1]) {
            uint8_t type = t[e], elen = t[e + 1];
            if (elen < 2) {
                break;
            }
            if (type == 0) {   // processor local APIC: uid, APIC id, flags (bit 0 enabled)
                uint32_t flags = get<uint32_t>(t, e + 4);
                if (verbose) kprintf("madt:   processor uid %u, APIC id %u, %s\n", t[e + 2], t[e + 3],
                        (flags & 1) ? "enabled" : "disabled");
                if ((flags & 1) && n < max) {
                    apic_ids[n++] = t[e + 3];
                }
            } else if (type == 1) {
                if (verbose) kprintf("madt:   I/O APIC id %u at %x, first GSI %u\n", t[e + 2],
                        get<uint32_t>(t, e + 4), get<uint32_t>(t, e + 8));
            } else if (type == 2) {
                if (verbose) kprintf("madt:   interrupt source override: IRQ %u -> GSI %u, flags %x\n",
                        t[e + 3], get<uint32_t>(t, e + 4), get<uint16_t>(t, e + 8));
            } else {
                if (verbose) kprintf("madt:   entry type %u, %u bytes\n", type, elen);
            }
        }
        return n;
    }
    return 0;
}

}  // namespace k
