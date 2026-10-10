// numa.cc - what the firmware tells an OS about the shape of a server: sockets and memory
// devices (SMBIOS types 4 and 17), processors (ACPI MADT), and which processors and memory
// ranges belong to which NUMA node (ACPI SRAT) and how far apart the nodes are (ACPI SLIT).
// A UEFI application built like F3-12; it only reads memory. Table layouts were written from
// memory of the ACPI and SMBIOS specifications and are checked only by checksums and by
// comparison with the QEMU command line (run.sh step 3); confirm them in the specifications.
#include "console.hpp"
#include "efi.hpp"

namespace {

uint16_t u16(const uint8_t* p, int o) { return static_cast<uint16_t>(p[o] | p[o + 1] << 8); }
uint32_t u32(const uint8_t* p, int o) { return u16(p, o) | static_cast<uint32_t>(u16(p, o + 2)) << 16; }
uint64_t u64(const uint8_t* p, int o) { return u32(p, o) | static_cast<uint64_t>(u32(p, o + 4)) << 32; }

const uint8_t* ptr(uint64_t physical)  // identity-mapped while boot services run (F3-09)
{
    return reinterpret_cast<const uint8_t*>(physical);
}

bool sums_to_zero(const uint8_t* p, uint32_t length)
{
    uint8_t sum = 0;
    for (uint32_t i = 0; i < length; ++i) {
        sum = static_cast<uint8_t>(sum + p[i]);
    }
    return sum == 0;
}

bool is(const uint8_t* table, const char* sig)
{
    return table[0] == sig[0] && table[1] == sig[1] && table[2] == sig[2] && table[3] == sig[3];
}

constexpr int kMaxNodes = 8;
uint64_t g_node_bytes[kMaxNodes] = {};
int g_node_cpus[kMaxNodes] = {};

void srat(Console& con, const uint8_t* t)
{
    // the SRAT body starts after the 36-byte header and 12 reserved bytes
    for (uint32_t o = 48; o + 2 <= u32(t, 4); o += t[o + 1]) {
        const uint8_t* e = t + o;
        if (e[1] == 0) {
            break;                                  // damaged entry: stop instead of looping
        }
        if (e[0] == 0) {                            // processor local APIC affinity
            const uint32_t node = e[2] | e[9] << 8 | e[10] << 16 | static_cast<uint32_t>(e[11]) << 24;
            const bool enabled = (u32(e, 4) & 1) != 0;
            con.print("  CPU   APIC ID ");
            con.dec(e[3]);
            con.print(" -> node ");
            con.dec(node);
            con.print(enabled ? "\n" : " (disabled)\n");
            if (enabled && node < kMaxNodes) {
                ++g_node_cpus[node];
            }
        } else if (e[0] == 1) {                     // memory affinity
            const uint32_t node = u32(e, 2);
            const uint64_t base = u64(e, 8);
            const uint64_t length = u64(e, 16);
            const bool enabled = (u32(e, 28) & 1) != 0;
            if (!enabled) {
                con.print("  MEM   (unused entry, flags 0)\n");
                continue;
            }
            con.print("  MEM   ");
            con.hex(base, 9);
            con.print(" + ");
            con.hex(length, 9);
            con.print(" (");
            con.dec(length / 1024);
            con.print(" KiB) -> node ");
            con.dec(node);
            con.print("\n");
            if (node < kMaxNodes) {
                g_node_bytes[node] += length;
            }
        } else {
            con.print("  entry type ");
            con.dec(e[0]);
            con.print("\n");
        }
    }
}

void slit(Console& con, const uint8_t* t)
{
    const uint64_t n = u64(t, 36);
    con.print("  relative distance, row = from node, column = to node\n");
    for (uint64_t from = 0; from < n; ++from) {
        con.print("  node ");
        con.dec(from);
        con.print(":");
        for (uint64_t to = 0; to < n; ++to) {
            con.print(" ");
            con.dec(t[44 + from * n + to]);
        }
        con.print("\n");
    }
}

const char* smbios_string(const uint8_t* s, uint8_t index)
{
    if (index == 0) {
        return "(none)";
    }
    const char* p = reinterpret_cast<const char*>(s + s[1]);
    for (uint8_t i = 1; i < index && *p != '\0'; ++i) {
        while (*p != '\0') {
            ++p;
        }
        ++p;
    }
    return p;
}

void smbios(Console& con, const uint8_t* ep)
{
    const uint8_t* s = ptr(u64(ep, 16));
    const uint8_t* end = s + u32(ep, 12);
    int sockets = 0;
    while (s + 4 <= end && s[0] != 127) {
        if (s[0] == 4) {
            ++sockets;
            con.print("  type 4  processor socket '");
            con.print(smbios_string(s, s[4]));
            con.print("'\n");
        } else if (s[0] == 17) {
            con.print("  type 17 memory device '");
            con.print(smbios_string(s, s[0x10]));
            con.print("', size field ");
            con.dec(u16(s, 0x0c) & 0x7fff);
            con.print((u16(s, 0x0c) & 0x8000) != 0 ? " KiB\n" : " MiB\n");
        }
        const uint8_t* p = s + s[1];                // skip the string set (ends with two zeros)
        while (p + 1 < end && (p[0] != 0 || p[1] != 0)) {
            ++p;
        }
        s = p + 2;
    }
    con.print("  processor sockets listed: ");
    con.dec(static_cast<uint64_t>(sockets));
    con.print("\n");
}

}  // namespace

extern "C" efi::Status efi_main(efi::Handle /*image*/, efi::SystemTable* st)
{
    Console con(st->con_out);
    con.print("F5-28 numa: start\n");
    const uint8_t* rsdp = nullptr;
    const uint8_t* sm3 = nullptr;
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        const efi::ConfigurationTable& e = st->configuration_table[i];
        if (efi::same(e.vendor_guid, efi::kAcpi20TableGuid)) {
            rsdp = static_cast<const uint8_t*>(e.vendor_table);
        } else if (efi::same(e.vendor_guid, efi::kSmbios3TableGuid)) {
            sm3 = static_cast<const uint8_t*>(e.vendor_table);
        }
    }
    if (sm3 != nullptr) {
        con.print("SMBIOS inventory:\n");
        smbios(con, sm3);
    }
    if (rsdp == nullptr) {
        con.print("no ACPI 2.0 RSDP\n");
        qemu_exit(0x11);
        return efi::kNotFound;
    }
    const uint8_t* xsdt = ptr(u64(rsdp, 24));
    const uint32_t entries = (u32(xsdt, 4) - 36) / 8;
    bool have_srat = false;
    for (uint32_t i = 0; i < entries; ++i) {
        const uint8_t* t = ptr(u64(xsdt, 36 + static_cast<int>(i) * 8));
        if (is(t, "APIC")) {
            int cpus = 0;
            for (uint32_t o = 44; o + 2 <= u32(t, 4) && t[o + 1] != 0; o += t[o + 1]) {
                cpus += (t[o] == 0) ? 1 : 0;
            }
            con.print("MADT: ");
            con.dec(static_cast<uint64_t>(cpus));
            con.print(" processor local APIC entries\n");
        } else if (is(t, "SRAT")) {
            have_srat = true;
            con.print("SRAT (checksum ");
            con.print(sums_to_zero(t, u32(t, 4)) ? "ok" : "BAD");
            con.print("):\n");
            srat(con, t);
        } else if (is(t, "SLIT")) {
            con.print("SLIT (checksum ");
            con.print(sums_to_zero(t, u32(t, 4)) ? "ok" : "BAD");
            con.print("):\n");
            slit(con, t);
        }
    }
    if (!have_srat) {
        con.print("no SRAT: the firmware describes no NUMA nodes (one node assumed)\n");
    }
    for (int n = 0; n < kMaxNodes; ++n) {
        if (g_node_cpus[n] != 0 || g_node_bytes[n] != 0) {
            con.print("node ");
            con.dec(static_cast<uint64_t>(n));
            con.print(": ");
            con.dec(static_cast<uint64_t>(g_node_cpus[n]));
            con.print(" CPUs, ");
            con.dec(g_node_bytes[n] / 1024);
            con.print(" KiB of memory\n");
        }
    }
    con.print("F5-28 numa: done\n");
    qemu_exit(0x10);
    return efi::kSuccess;
}
