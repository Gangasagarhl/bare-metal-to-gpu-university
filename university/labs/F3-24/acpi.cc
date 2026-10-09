// acpi.cc - F3-24: RSDP search, root-table walk, checksum validation.
#include "acpi.h"
#include <cstdint>
#include "kprint.h"
#include "log.h"
#include "pmm.h"

namespace acpi {
namespace {

struct [[gnu::packed]] Rsdp {
    char signature[8];              // "RSD PTR " (with the trailing space)
    uint8_t checksum;               // covers the first 20 bytes
    char oem_id[6];
    uint8_t revision;               // 0 = ACPI 1.0 (RSDT only), 2 = ACPI 2.0 or later
    uint32_t rsdt;
    uint32_t length;                // revision 2 and later: the whole structure
    uint64_t xsdt;
    uint8_t extended_checksum;      // covers all 'length' bytes
    uint8_t reserved[3];
};
static_assert(sizeof(Rsdp) == 36);

constexpr int kMaxTables = 32;
const Header* g_tables[kMaxTables];
int g_count = 0;
uint64_t g_rsdp = 0;

const uint8_t* at(uint64_t phys) { return static_cast<const uint8_t*>(pmm::phys_to_virt(phys)); }

bool same(const char* a, const char* b, int n)
{
    for (int i = 0; i < n; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

uint64_t scan(uint64_t from, uint64_t to)      // the RSDP sits on a 16-byte boundary
{
    for (uint64_t a = from; a + sizeof(Rsdp) <= to; a += 16) {
        const auto* r = reinterpret_cast<const Rsdp*>(at(a));
        if (same(r->signature, "RSD PTR ", 8) && checksum_ok(r, 20)) {
            return a;
        }
    }
    return 0;
}

} // namespace

uint64_t rsdp_phys() { return g_rsdp; }

bool init()
{
    g_count = 0;
    // 1. the first KiB of the EBDA (its segment is the 16-bit word at physical 0x40E), 2. 0xE0000-0xFFFFF
    uint64_t ebda = uint64_t{*reinterpret_cast<const uint16_t*>(at(0x40E))} << 4;
    g_rsdp = ebda >= 0x80000 && ebda < 0xA0000 ? scan(ebda, ebda + 1024) : 0;
    if (g_rsdp == 0) {
        g_rsdp = scan(0xE0000, 0x100000);
    }
    if (g_rsdp == 0) {
        klog(Level::Error, "acpi: no RSDP found");
        return false;
    }
    const auto* r = reinterpret_cast<const Rsdp*>(at(g_rsdp));
    bool use_xsdt = r->revision >= 2 && checksum_ok(r, r->length) && r->xsdt != 0;
    klog(Level::Info, "acpi: RSDP at %05lx, revision %u, OEM '%.6s', RSDT %08x%s", g_rsdp,
         unsigned{r->revision}, r->oem_id, r->rsdt, use_xsdt ? ", XSDT present" : " (no XSDT: using the RSDT)");
    uint64_t root = use_xsdt ? r->xsdt : r->rsdt;
    const auto* rh = reinterpret_cast<const Header*>(at(root));
    if (!checksum_ok(rh, rh->length)) {
        klog(Level::Error, "acpi: %.4s at %08lx has a bad checksum", rh->signature, root);
        return false;
    }
    unsigned entry_size = use_xsdt ? 8 : 4;
    unsigned n = (rh->length - sizeof(Header)) / entry_size;
    klog(Level::Info, "acpi: %.4s at %08lx, %u bytes, %u entries", rh->signature, root, rh->length, n);
    const uint8_t* entries = at(root) + sizeof(Header);
    for (unsigned i = 0; i < n && g_count < kMaxTables; ++i) {
        uint64_t p = use_xsdt ? *reinterpret_cast<const uint64_t*>(entries + 8 * i)
                              : *reinterpret_cast<const uint32_t*>(entries + 4 * i);
        const auto* h = reinterpret_cast<const Header*>(at(p));
        bool ok = checksum_ok(h, h->length);
        klog(ok ? Level::Info : Level::Error, "acpi:   %.4s at %08lx  %5u bytes  rev %u  OEM '%.6s'  checksum %s",
             h->signature, p, h->length, unsigned{h->revision}, h->oem_id, ok ? "ok" : "BAD: table rejected");
        if (ok) {
            g_tables[g_count++] = h;
        }
        if (ok && same(h->signature, "FACP", 4) && h->length >= 44) {
            // the DSDT is reached through the FADT, not the root table (32-bit field at offset 40)
            uint64_t d = *reinterpret_cast<const uint32_t*>(reinterpret_cast<const uint8_t*>(h) + 40);
            const auto* dh = reinterpret_cast<const Header*>(at(d));
            bool dok = checksum_ok(dh, dh->length);
            klog(dok ? Level::Info : Level::Error, "acpi:   %.4s at %08lx  %5u bytes  rev %u  OEM '%.6s'  checksum %s (via FACP)",
                 dh->signature, d, dh->length, unsigned{dh->revision}, dh->oem_id, dok ? "ok" : "BAD: table rejected");
            if (dok && g_count < kMaxTables) {
                g_tables[g_count++] = dh;
            }
        }
    }
    return true;
}

const Header* find(const char sig[4])
{
    for (int i = 0; i < g_count; ++i) {
        if (same(g_tables[i]->signature, sig, 4)) {
            return g_tables[i];
        }
    }
    return nullptr;
}

} // namespace acpi
