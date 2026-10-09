// ebbrprobe.cc - F4-33: what does the firmware hand to an OS loader on an Arm system?
//
// A UEFI application (PE32+ for AArch64) that lists the firmware's configuration tables and
// recognises each one by its CONTENT (devicetree magic, ACPI "RSD PTR ", SMBIOS anchors), then
// reads the hardware description it found: devicetree nodes through ../F4-31/fdt.h, or the list
// of ACPI tables through the XSDT. This is the question EBBR and SystemReady answer: where does
// a generic OS image learn about the board?
#include "../F4-31/fdt.h"
#include "../F4-31/kbase.h"
#include "efi_a64.h"

namespace {
efi::TextOut* g_out = nullptr;

// The GUID under which this firmware published its devicetree, copied from this lab's own run
// (probe_dt.out). The UEFI and EBBR documents define the name and value of this GUID; check
// them before relying on it (not opened during this build).
constexpr efi::Guid kDtbGuid{0xb1b621d5, 0xf19c, 0x41a5, {0x83, 0x0b, 0xd9, 0x15, 0x2c, 0x69, 0xaa, 0xe0}};

bool same(const efi::Guid& x, const efi::Guid& y)
{
    const auto* a = reinterpret_cast<const uint8_t*>(&x);
    const auto* b = reinterpret_cast<const uint8_t*>(&y);
    for (int i = 0; i < 16; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

void uefi_putc(char c)
{
    char16_t s[2] = {static_cast<char16_t>(static_cast<unsigned char>(c)), u'\0'};
    g_out->output_string(g_out, s);
}

bool starts_with(const void* p, const char* sig)
{
    const auto* b = static_cast<const char*>(p);
    for (int i = 0; sig[i] != '\0'; ++i) {
        if (b[i] != sig[i]) {
            return false;
        }
    }
    return true;
}

uint32_t le32(const uint8_t* p) { return p[0] | (p[1] << 8) | (p[2] << 16) | (uint32_t{p[3]} << 24); }
uint64_t le64(const uint8_t* p) { return le32(p) | (uint64_t{le32(p + 4)} << 32); }

void print_guid(const efi::Guid& g)
{
    k::printf("%08x-%04x-%04x-%02x%02x-", g.a, g.b, g.c, g.d[0], g.d[1]);
    for (int i = 2; i < 8; ++i) {
        k::printf("%02x", g.d[i]);
    }
}

// ACPI: RSDP -> XSDT -> one 8-byte pointer per table; each table starts with a 4-character
// signature and a 32-bit length (ACPI Specification, "Root System Description Pointer" and
// "Extended System Description Table" - title only, pending verification).
void list_acpi(const uint8_t* rsdp)
{
    k::printf("    RSDP revision %u, OEM \"%c%c%c%c%c%c\"\n", rsdp[15], rsdp[9], rsdp[10], rsdp[11],
              rsdp[12], rsdp[13], rsdp[14]);
    if (rsdp[15] < 2) {
        return;
    }
    const auto* xsdt = reinterpret_cast<const uint8_t*>(le64(rsdp + 24));
    uint32_t len = le32(xsdt + 4);
    k::printf("    XSDT at %p lists %u tables:", static_cast<const void*>(xsdt), (len - 36) / 8);
    for (uint32_t off = 36; off + 8 <= len; off += 8) {
        const auto* t = reinterpret_cast<const char*>(le64(xsdt + off));
        k::printf(" %c%c%c%c", t[0], t[1], t[2], t[3]);
    }
    k::printf("\n");
}

void describe_dt(const void* blob)
{
    fdt::Tree t;
    const char* err = nullptr;
    if (!t.init(blob, &err)) {
        k::printf("    devicetree rejected: %s\n", err);
        return;
    }
    fdt::Prop p;
    if (t.prop(t.root(), "compatible", p)) {
        k::printf("    root compatible \"%s\", %u bytes\n", reinterpret_cast<const char*>(p.data), t.total_size());
    }
    int cpus = 0;
    int devs = 0;
    int cpus_node = t.find_path("/cpus");
    for (int c = cpus_node == fdt::kNone ? fdt::kNone : t.first_child(cpus_node); c != fdt::kNone;
         c = t.next_sibling(c)) {
        cpus += t.prop(c, "device_type", p) ? 1 : 0;
    }
    for (int n = t.root(); n != fdt::kNone; n = t.next_node(n)) {
        devs += (t.prop(n, "compatible", p) && t.available(n)) ? 1 : 0;
    }
    int chosen = t.find_path("/chosen");
    const char* out = (chosen != fdt::kNone && t.prop(chosen, "stdout-path", p))
                          ? reinterpret_cast<const char*>(p.data) : "(none)";
    k::printf("    %d CPU nodes, %d enabled nodes with a compatible, /chosen stdout-path \"%s\"\n", cpus, devs, out);
    k::printf("    /memory node present: %s (UEFI's memory map is the one to use; see below)\n",
              t.find_path("/memory") != fdt::kNone || t.find_path("/memory@40000000") != fdt::kNone ? "yes" : "no");
}

void memory_summary(efi::BootServices* bs)
{
    uint64_t size = 0;
    uint64_t key = 0;
    uint64_t dsize = 0;
    uint32_t dver = 0;
    bs->get_memory_map(&size, nullptr, &key, &dsize, &dver);   // asks for the size only
    size += 4 * dsize;
    void* buf = nullptr;
    if (bs->allocate_pool(efi::kLoaderData, size, &buf) != efi::kSuccess ||
        bs->get_memory_map(&size, static_cast<efi::MemoryDescriptor*>(buf), &key, &dsize, &dver) != efi::kSuccess) {
        k::printf("memory map: could not read it\n");
        return;
    }
    uint64_t free_pages = 0;
    uint64_t lowest = ~0ull;
    uint64_t n = size / dsize;
    for (uint64_t i = 0; i < n; ++i) {
        const auto* d = reinterpret_cast<const efi::MemoryDescriptor*>(static_cast<uint8_t*>(buf) + i * dsize);
        if (d->type == efi::kConventionalMemory) {
            free_pages += d->number_of_pages;
            lowest = d->physical_start < lowest ? d->physical_start : lowest;
        }
    }
    using ull = unsigned long long;
    k::printf("memory map: %llu descriptors of %llu bytes; free (conventional) memory %llu MiB, lowest at 0x%llx\n",
              ull{n}, ull{dsize}, ull{free_pages * 4096 >> 20}, ull{lowest});
    bs->free_pool(buf);
}
}  // namespace

extern "C" efi::Status efi_main(efi::Handle, efi::SystemTable* st)
{
    g_out = st->con_out;
    k::set_console(uefi_putc);
    k::printf("ebbrprobe: UEFI revision %u.%u, firmware vendor \"", st->hdr.revision >> 16,
              (st->hdr.revision & 0xffff) / 10);
    for (const char16_t* v = st->firmware_vendor; *v != u'\0'; ++v) {
        k::printf("%c", static_cast<char>(*v));
    }
    k::printf("\", %llu configuration tables\n", static_cast<unsigned long long>(st->number_of_table_entries));
    int dt = 0;
    int acpi = 0;
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        const efi::ConfigurationTable& ct = st->configuration_table[i];
        const auto* p = static_cast<const uint8_t*>(ct.vendor_table);
        k::printf("  [%llu] ", static_cast<unsigned long long>(i));
        print_guid(ct.vendor_guid);
        if (fdt::be32(p) == fdt::kMagic) {
            k::printf("  devicetree (FDT magic 0xd00dfeed)\n");
            describe_dt(p);
            ++dt;
        } else if (starts_with(p, "RSD PTR ")) {
            k::printf("  ACPI RSDP (\"RSD PTR \")\n");
            list_acpi(p);
            ++acpi;
        } else if (starts_with(p, "_SM3_")) {
            k::printf("  SMBIOS 3 entry point (\"_SM3_\")\n");
        } else if (starts_with(p, "_SM_")) {
            k::printf("  SMBIOS entry point (\"_SM_\")\n");
        } else {
            k::printf("  (other firmware table)\n");
        }
    }
    k::printf("hardware description handed over: %s\n",
              dt != 0 && acpi != 0 ? "devicetree AND ACPI" : dt != 0 ? "devicetree" : acpi != 0 ? "ACPI" : "NONE");
    // What a real loader does: look the devicetree up by its GUID, not by scanning contents.
    const void* by_guid = nullptr;
    for (uint64_t i = 0; i < st->number_of_table_entries; ++i) {
        if (same(st->configuration_table[i].vendor_guid, kDtbGuid)) {
            by_guid = st->configuration_table[i].vendor_table;
        }
    }
    k::printf("lookup of the devicetree by GUID: %s\n", by_guid != nullptr ? "found" : "not found");
    memory_summary(st->boot_services);
    k::printf("ebbrprobe: done\n");
    return efi::kSuccess;
}
