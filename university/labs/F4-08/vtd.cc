// vtd.cc - DR302 F4-08: DMAR parsing, root/context tables, three-level second-level page
// tables (39-bit guest address width), register-based invalidation, fault recording.
#include "vtd.h"
#include "intr.h"
#include "msi.h"
#include "../F4-01/kbase.h"
#include "../F4-02/acpi.h"

using namespace vtdreg;

namespace {
uintptr_t g_base = 0;
uint64_t g_cap = 0, g_ecap = 0;

struct alignas(4096) Page { uint64_t e[512]; };
static_assert(sizeof(Page) == 4096, "a table is one 4 KiB page");
Page g_root;                         // 256 root entries x 16 bytes (one per bus)
Page g_context[1];                   // bus 0 only: 256 context entries x 16 bytes
Page g_pool[48];                     // page-table pages handed out on demand
int g_pool_used = 0;
uint64_t g_slpt[256];                // per device-function: physical address of its top table

uint32_t rd32(uint32_t off) { return mmio_read<uint32_t>(g_base + off); }
void wr32(uint32_t off, uint32_t v) { mmio_write<uint32_t>(g_base + off, v); }
uint64_t rd64(uint32_t off) { return rd32(off) | (uint64_t{rd32(off + 4)} << 32); }
void wr64(uint32_t off, uint64_t v)
{
    wr32(off, static_cast<uint32_t>(v));
    wr32(off + 4, static_cast<uint32_t>(v >> 32));
}

Page* new_table()
{
    if (g_pool_used == 48) panic("vtd: page-table pool exhausted");
    Page* p = &g_pool[g_pool_used++];
    memset(p, 0, sizeof *p);
    return p;
}

uint8_t devfn(PciAddr d) { return static_cast<uint8_t>(d.dev << 3 | d.fn); }

// Walks (and, if 'create', builds) the 3-level table for 'iova'; returns the leaf entry.
uint64_t* leaf(PciAddr dev, uint64_t iova, bool create)
{
    uint64_t table = g_slpt[devfn(dev)];
    if (!table) return nullptr;
    for (int level = 3; level >= 2; --level) {
        const unsigned idx = (iova >> (12 + 9 * (level - 1))) & 0x1FF;
        uint64_t& e = reinterpret_cast<Page*>(static_cast<uintptr_t>(table))->e[idx];
        if (!(e & (PTE_R | PTE_W))) {
            if (!create) return nullptr;
            e = reinterpret_cast<uintptr_t>(new_table()) | PTE_R | PTE_W;
        }
        table = e & PTE_ADDR;
    }
    return &reinterpret_cast<Page*>(static_cast<uintptr_t>(table))->e[(iova >> 12) & 0x1FF];
}

void command(uint32_t bit)
{
    // GCMD is write-only; the persistent bits we want kept are read back from GSTS.
    const uint32_t keep = rd32(GSTS) & GCMD_TE;
    wr32(GCMD, keep | bit);
    for (int guard = 0; (rd32(GSTS) & bit) == 0; ++guard)
        if (guard > 1000000) panic("vtd: command not acknowledged");
}
}  // namespace

namespace vtd {
bool present() { return g_base != 0; }

bool init()
{
    const AcpiHeader* d = acpi::find("DMAR");
    if (!d) { kprintf("vtd: no DMAR table: no IOMMU on this machine\n"); return false; }
    const auto* p = reinterpret_cast<const uint8_t*>(d);
    kprintf("vtd: DMAR length %u, host address width %u bits, flags 0x%x\n", d->length, p[36] + 1u, p[37]);
    // Remapping structures follow the 48-byte header; type 0 = DRHD (a remapping unit).
    for (uint32_t off = 48; off + 4 <= d->length;) {
        uint16_t type, len;
        memcpy(&type, p + off, 2);
        memcpy(&len, p + off + 2, 2);
        if (len < 4) break;
        if (type == 0) {
            uint64_t base;
            memcpy(&base, p + off + 8, 8);
            kprintf("vtd: DRHD flags 0x%x (INCLUDE_PCI_ALL=%u) segment %u register base 0x%lx\n",
                    p[off + 4], p[off + 4] & 1, p[off + 6] | (p[off + 7] << 8), base);
            if (!g_base) g_base = static_cast<uintptr_t>(base);
            // Device scopes: type, length, 2 reserved, enumeration id, start bus, path pairs.
            for (uint32_t s = off + 16; s + 6 <= off + len && p[s + 1] >= 6; s += p[s + 1]) {
                static const char* const kinds[] = {"?", "PCI endpoint", "PCI sub-hierarchy",
                                                    "IOAPIC", "HPET", "ACPI device"};
                kprintf("vtd:   scope %s, enumeration id %u, bus %u, path", p[s] <= 5 ? kinds[p[s]] : "?",
                        p[s + 4], p[s + 5]);
                for (uint32_t q = s + 6; q + 2 <= s + p[s + 1]; q += 2) kprintf(" %02x.%x", p[q], p[q + 1]);
                kprintf("\n");
            }
        } else {
            kprintf("vtd: remapping structure type %u, %u bytes (not used here)\n", type, len);
        }
        off += len;
    }
    if (!g_base) return false;
    g_cap = rd64(CAP);
    g_ecap = rd64(ECAP);
    const uint32_t ver = rd32(VER);
    kprintf("vtd: version %u.%u CAP 0x%lx ECAP 0x%lx\n", (ver >> 4) & 0xF, ver & 0xF, g_cap, g_ecap);
    kprintf("vtd: SAGAW 0x%x (39-bit 3-level %s), MGAW %u, domains 2^%u, caching mode %u, "
            "fault records %u at +0x%x, IOTLB regs at +0x%x\n", cap_sagaw(g_cap),
            (cap_sagaw(g_cap) & 2) ? "supported" : "missing", cap_mgaw(g_cap), 4 + 2 * cap_nd(g_cap),
            cap_cm(g_cap) ? 1 : 0, cap_nfr(g_cap), cap_fro(g_cap), ecap_iro(g_ecap));
    if (!(cap_sagaw(g_cap) & 2)) return false;
    memset(&g_root, 0, sizeof g_root);
    memset(&g_context[0], 0, sizeof g_context[0]);
    g_root.e[0] = reinterpret_cast<uintptr_t>(&g_context[0]) | 1;   // bus 0 present
    return true;
}

void attach(PciAddr dev, uint16_t domain_id)
{
    if (dev.bus != 0) panic("vtd: this teaching driver handles bus 0 only");
    Page* top = new_table();
    g_slpt[devfn(dev)] = reinterpret_cast<uintptr_t>(top);
    uint64_t* ce = &g_context[0].e[2 * devfn(dev)];
    ce[1] = 1u | (uint64_t{domain_id} << 8);          // AW = 001: 39-bit, 3-level; DID
    ce[0] = reinterpret_cast<uintptr_t>(top) | 1;     // TT = 00 (translate), present
    kprintf("vtd: %02x:%02x.%x -> domain %u, context entry 0x%016lx_%016lx\n", dev.bus, dev.dev,
            dev.fn, domain_id, ce[1], ce[0]);
}

void map_page(PciAddr dev, uint64_t iova, uint64_t phys, bool writable)
{
    uint64_t* e = leaf(dev, iova, true);
    *e = (phys & PTE_ADDR) | PTE_R | (writable ? PTE_W : 0);
}

void unmap_page(PciAddr dev, uint64_t iova)
{
    uint64_t* e = leaf(dev, iova, false);
    if (e) *e = 0;
}

void flush_iotlb()
{
    const uint32_t r = ecap_iro(g_ecap) + 8;           // IOTLB_REG follows IVA_REG
    wr64(r, IOTLB_IVT | IOTLB_GLOBAL);
    for (int guard = 0; rd64(r) & IOTLB_IVT; ++guard)
        if (guard > 1000000) panic("vtd: IOTLB invalidation timed out");
}

void enable()
{
    wr64(RTADDR, reinterpret_cast<uintptr_t>(&g_root));   // TTM = 00: legacy tables
    command(GCMD_SRTP);
    wr64(CCMD, CCMD_ICC | CCMD_GLOBAL);
    for (int guard = 0; rd64(CCMD) & CCMD_ICC; ++guard)
        if (guard > 1000000) panic("vtd: context-cache invalidation timed out");
    flush_iotlb();
    command(GCMD_TE);
    kprintf("vtd: root table at 0x%x, translation enabled (GSTS 0x%08x)\n",
            reinterpret_cast<uintptr_t>(&g_root), rd32(GSTS));
}

void fault_irq(uint8_t vector)
{
    const MsiMessage m = msi_message(vector, intr::lapic_id());
    wr32(FEDATA, m.data);
    wr32(FEADDR, m.addr_lo);
    wr32(FEUADDR, m.addr_hi);
    wr32(FECTL, 0);                                    // IM = 0: fault events unmasked
}

int read_faults(VtdFault* out, int max)
{
    int n = 0;
    const uint32_t fro = cap_fro(g_cap);
    for (uint32_t i = 0; i < cap_nfr(g_cap); ++i) {
        const uint32_t r = fro + 16 * i;
        const uint64_t hi = rd64(r + 8);
        if (!(hi >> 63)) continue;                     // F bit clear: no fault in this record
        if (n < max) {
            out[n].addr = rd64(r) & ~0xFFFull;
            out[n].source_id = static_cast<uint16_t>(hi & 0xFFFF);
            out[n].reason = static_cast<uint8_t>((hi >> 32) & 0xFF);
            out[n].read = (hi >> 62) & 1;
            out[n].raw_lo = rd64(r);
            out[n].raw_hi = hi;
            ++n;
        }
        wr32(r + 12, 1u << 31);                        // write 1 to F to clear the record
    }
    wr32(FSTS, FSTS_PFO | FSTS_PPF);                   // clear overflow and pending (RW1C)
    return n;
}

const char* reason_text(uint8_t r)
{
    switch (r) {
    case 1: return "root entry not present";
    case 2: return "context entry not present";
    case 3: return "invalid context entry";
    case 4: return "address beyond the guest address width";
    case 5: return "write to a page without write permission";
    case 6: return "read from a page without read permission";
    default: return "other (see the specification's fault-reason table)";
    }
}

void dump_walk(PciAddr dev, uint64_t iova)
{
    uint64_t table = g_slpt[devfn(dev)];
    kprintf("walk: IOVA 0x%lx -> indices L3=%u L2=%u L1=%u offset 0x%x\n", iova,
            static_cast<uint32_t>((iova >> 30) & 0x1FF), static_cast<uint32_t>((iova >> 21) & 0x1FF),
            static_cast<uint32_t>((iova >> 12) & 0x1FF), static_cast<uint32_t>(iova & 0xFFF));
    for (int level = 3; level >= 1 && table; --level) {
        const unsigned idx = (iova >> (12 + 9 * (level - 1))) & 0x1FF;
        const uint64_t e = reinterpret_cast<Page*>(static_cast<uintptr_t>(table))->e[idx];
        kprintf("walk:   level %d table 0x%lx [%u] = 0x%016lx%s\n", level, table, idx, e,
                (e & (PTE_R | PTE_W)) ? "" : "  (not present)");
        if (!(e & (PTE_R | PTE_W))) return;
        table = level > 1 ? (e & PTE_ADDR) : 0;
        if (level == 1)
            kprintf("walk:   -> physical 0x%lx (%s)\n", (e & PTE_ADDR) | (iova & 0xFFF),
                    (e & PTE_W) ? "read/write" : "read only");
    }
}
}  // namespace vtd
