// gic.cc - F4-25: GICv3 on QEMU virt (gic-version=3). GICv2 is detected and refused.
#include "gic.h"
#include "arch.h"
#include "kprint.h"

namespace {

uint64_t g_gicd = 0;        // distributor base
uint64_t g_gicr = 0;        // first redistributor frame
uint64_t g_gicr_size = 0;
const char* g_compat = "(none)";

// Distributor registers (offsets from GICD base)
constexpr uint64_t GICD_CTLR = 0x0000, GICD_TYPER = 0x0004, GICD_IGROUPR = 0x0080,
                   GICD_ISENABLER = 0x0100, GICD_IPRIORITYR = 0x0400, GICD_ICFGR = 0x0c00,
                   GICD_IROUTER = 0x6000;
constexpr uint32_t CTLR_ARE = 1u << 4, CTLR_ENABLE_GRP1 = 1u << 1, CTLR_RWP = 1u << 31;
// Redistributor: two 64 KiB frames per CPU (RD_base, then SGI_base)
constexpr uint64_t GICR_TYPER = 0x0008, GICR_WAKER = 0x0014, GICR_FRAME = 0x20000, SGI_BASE = 0x10000;
constexpr uint64_t GICR_IGROUPR0 = 0x0080, GICR_ISENABLER0 = 0x0100, GICR_ISPENDR0 = 0x0200,
                   GICR_ISACTIVER0 = 0x0300, GICR_IPRIORITYR = 0x0400;
constexpr uint32_t WAKER_PROCESSOR_SLEEP = 1u << 1, WAKER_CHILDREN_ASLEEP = 1u << 2;

uint64_t g_my_rd[8];        // this CPU's redistributor RD_base, indexed by CPU number (Aff0)

uint32_t r32(uint64_t a) { return *reinterpret_cast<volatile uint32_t*>(a); }
uint64_t r64(uint64_t a) { return *reinterpret_cast<volatile uint64_t*>(a); }
void w32(uint64_t a, uint32_t v) { *reinterpret_cast<volatile uint32_t*>(a) = v; }
void w64(uint64_t a, uint64_t v) { *reinterpret_cast<volatile uint64_t*>(a) = v; }
void w8(uint64_t a, uint8_t v) { *reinterpret_cast<volatile uint8_t*>(a) = v; }

uint64_t cpu_index() { return READ_SYSREG(mpidr_el1) & 0xff; }

// MPIDR affinity fields packed the way GICR_TYPER bits 63:32 hold them: Aff3.Aff2.Aff1.Aff0
uint32_t my_affinity()
{
    uint64_t m = READ_SYSREG(mpidr_el1);
    return static_cast<uint32_t>(((m >> 32) & 0xff) << 24 | (m & 0xffffff));
}

void wait_rwp()
{
    while (r32(g_gicd + GICD_CTLR) & CTLR_RWP) {
    }
}

} // namespace

namespace gic {

Version probe(const fdt::Blob& dt)
{
    fdt::Node n;
    uint64_t size = 0;
    if (dt.find_compatible("arm,gic-v3", &n)) {
        g_compat = "arm,gic-v3";
        dt.reg(n, 0, &g_gicd, &size);
        dt.reg(n, 1, &g_gicr, &g_gicr_size);
        return Version::V3;
    }
    static const char* const kV2[] = {"arm,cortex-a15-gic", "arm,gic-400", "arm,cortex-a9-gic"};
    for (const char* c : kV2) {
        if (dt.find_compatible(c, &n)) {
            g_compat = c;
            return Version::V2;
        }
    }
    return Version::None;
}

const char* compatible()
{
    return g_compat;
}

void init_distributor()
{
    w32(g_gicd + GICD_CTLR, 0);
    wait_rwp();
    uint32_t typer = r32(g_gicd + GICD_TYPER);
    kprintf("GICD at 0x%lx: TYPER 0x%x -> %u interrupt IDs; GICR region 0x%lx (0x%lx bytes)\n", g_gicd, typer,
            32 * ((typer & 0x1f) + 1), g_gicr, g_gicr_size);
    w32(g_gicd + GICD_CTLR, CTLR_ARE | CTLR_ENABLE_GRP1);   // affinity routing, group 1 on
    wait_rwp();
}

bool init_cpu()
{
    // find this CPU's redistributor: the frame whose TYPER affinity equals our MPIDR
    uint64_t rd = 0;
    for (uint64_t f = g_gicr; f < g_gicr + g_gicr_size; f += GICR_FRAME) {
        uint64_t typer = r64(f + GICR_TYPER);
        if (static_cast<uint32_t>(typer >> 32) == my_affinity()) {
            rd = f;
            break;
        }
        if (typer & (1u << 4)) {   // Last: no more frames
            break;
        }
    }
    if (rd == 0) {
        return false;
    }
    g_my_rd[cpu_index() & 7] = rd;
    w32(rd + GICR_WAKER, r32(rd + GICR_WAKER) & ~WAKER_PROCESSOR_SLEEP);   // wake it up
    while (r32(rd + GICR_WAKER) & WAKER_CHILDREN_ASLEEP) {
    }
    w32(rd + SGI_BASE + GICR_IGROUPR0, 0xffffffff);   // SGIs and PPIs: group 1 (non-secure)
    // CPU interface through system registers
    WRITE_SYSREG(icc_sre_el1, READ_SYSREG(icc_sre_el1) | 1);   // SRE = 1: system-register interface
    arch::isb();
    WRITE_SYSREG(icc_pmr_el1, 0xff);    // let every priority through
    WRITE_SYSREG(icc_bpr1_el1, 0);      // no priority grouping
    WRITE_SYSREG(icc_igrpen1_el1, 1);   // group 1 interrupts on
    arch::isb();
    return true;
}

void enable_ppi(uint32_t intid, uint8_t priority)
{
    uint64_t sgi = g_my_rd[cpu_index() & 7] + SGI_BASE;
    w8(sgi + GICR_IPRIORITYR + intid, priority);
    w32(sgi + GICR_ISENABLER0, 1u << intid);
}

void enable_spi(uint32_t intid, uint8_t priority, bool level)
{
    uint64_t bit = 1u << (intid % 32);
    w32(g_gicd + GICD_IGROUPR + 4 * (intid / 32), r32(g_gicd + GICD_IGROUPR + 4 * (intid / 32)) | bit);
    w8(g_gicd + GICD_IPRIORITYR + intid, priority);
    uint64_t cfg = g_gicd + GICD_ICFGR + 4 * (intid / 16);
    uint32_t shift = 2 * (intid % 16) + 1;   // Int_config bit: 0 = level, 1 = edge
    w32(cfg, level ? (r32(cfg) & ~(1u << shift)) : (r32(cfg) | (1u << shift)));
    w64(g_gicd + GICD_IROUTER + 8 * intid, READ_SYSREG(mpidr_el1) & 0xff00ffffffull);   // to this CPU
    w32(g_gicd + GICD_ISENABLER + 4 * (intid / 32), static_cast<uint32_t>(bit));
}

void send_sgi(uint32_t intid, uint64_t target_mpidr)
{
    // ICC_SGI1R_EL1: INTID in bits 27:24, Aff1 in 23:16, target list (bit per Aff0) in 15:0
    uint64_t v = (uint64_t{intid} << 24) | (((target_mpidr >> 8) & 0xff) << 16) | (1u << (target_mpidr & 0xf));
    WRITE_SYSREG(icc_sgi1r_el1, v);
    arch::isb();
}

uint32_t ack()
{
    uint32_t id = static_cast<uint32_t>(READ_SYSREG(icc_iar1_el1)) & 0xffffff;
    asm volatile("dsb sy" : : : "memory");
    return id;
}

void eoi(uint32_t intid)
{
    WRITE_SYSREG(icc_eoir1_el1, intid);
    arch::isb();
}

void dump()
{
    uint64_t sgi = g_my_rd[cpu_index() & 7] + SGI_BASE;
    kprintf("GIC state: GICR_ISENABLER0 0x%x ISPENDR0 0x%x ISACTIVER0 0x%x; ICC_RPR_EL1 0x%lx\n",
            r32(sgi + GICR_ISENABLER0), r32(sgi + GICR_ISPENDR0), r32(sgi + GICR_ISACTIVER0),
            READ_SYSREG(icc_rpr_el1));
}

} // namespace gic
