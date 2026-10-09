// plic.cc - F4-29: PLIC driver; contexts discovered from the devicetree.
#include "plic.h"
#include "kprint.h"

namespace {

constexpr int kMaxHarts = 8;
constexpr uint32_t kSupervisorExternal = 9;   // interrupt cause number in interrupts-extended
uint64_t g_base = 0;
int g_ctx[kMaxHarts];                          // S-mode context per hart ID, or -1

volatile uint32_t& reg(uint64_t off) { return *reinterpret_cast<volatile uint32_t*>(g_base + off); }
uint64_t enable_off(int ctx, uint32_t src) { return 0x2000 + 0x80 * static_cast<uint64_t>(ctx) + 4 * (src / 32); }
uint64_t threshold_off(int ctx) { return 0x200000 + 0x1000 * static_cast<uint64_t>(ctx); }
uint64_t claim_off(int ctx) { return threshold_off(ctx) + 4; }

} // namespace

namespace plic {

bool probe(const fdt::Blob& dt)
{
    for (int& c : g_ctx) {
        c = -1;
    }
    fdt::Node n;
    uint64_t size = 0;
    if (!dt.find_compatible("riscv,plic0", &n) || !dt.reg(n, 0, &g_base, &size)) {
        return false;
    }
    // Map each hart's interrupt-controller phandle to the hart ID (its parent cpu node's reg).
    uint32_t ph_of_hart[kMaxHarts] = {};
    int cpu_depth = -1;
    uint64_t cpu_hart = 0;
    dt.for_each_node([&](const fdt::Node& x) {
        fdt::Prop p;
        uint64_t hart = 0, unused = 0;
        if (dt.get_prop(x, "device_type", &p) && fdt::streq(reinterpret_cast<const char*>(p.data), "cpu") &&
            dt.reg(x, 0, &hart, &unused)) {
            cpu_depth = x.depth;
            cpu_hart = hart;
        } else if (x.depth == cpu_depth + 1 && dt.is_compatible(x, "riscv,cpu-intc") &&
                   dt.get_prop(x, "phandle", &p) && cpu_hart < kMaxHarts) {
            ph_of_hart[cpu_hart] = fdt::be32(p.data);
        } else if (x.depth <= cpu_depth) {
            cpu_depth = -1;
        }
        return true;
    });
    // interrupts-extended = <phandle irq> per context, in context order
    fdt::Prop ie;
    if (!dt.get_prop(n, "interrupts-extended", &ie)) {
        return false;
    }
    for (uint32_t ctx = 0; 8 * ctx + 8 <= ie.len; ++ctx) {
        uint32_t ph = fdt::be32(ie.data + 8 * ctx), irq = fdt::be32(ie.data + 8 * ctx + 4);
        for (int h = 0; h < kMaxHarts; ++h) {
            if (ph_of_hart[h] == ph && ph != 0 && irq == kSupervisorExternal) {
                g_ctx[h] = static_cast<int>(ctx);
            }
        }
    }
    kprintf("PLIC at 0x%lx; S-mode contexts by hart:", g_base);
    for (int h = 0; h < kMaxHarts; ++h) {
        if (ph_of_hart[h] != 0) {
            kprintf(" hart %d -> %d", h, g_ctx[h]);
        }
    }
    kprintf("\n");
    return true;
}

int s_context(uint64_t hartid)
{
    return hartid < kMaxHarts ? g_ctx[hartid] : -1;
}

void init_hart(uint64_t hartid)
{
    int ctx = s_context(hartid);
    if (ctx >= 0) {
        reg(threshold_off(ctx)) = 0;   // accept every priority above 0
    }
}

void enable(uint64_t hartid, uint32_t source, uint32_t priority)
{
    int ctx = s_context(hartid);
    if (ctx < 0) {
        return;
    }
    reg(4 * uint64_t{source}) = priority;                  // source priority (0 = never)
    reg(enable_off(ctx, source)) = reg(enable_off(ctx, source)) | (1u << (source % 32));
}

uint32_t claim(uint64_t hartid)
{
    int ctx = s_context(hartid);
    return ctx < 0 ? 0 : reg(claim_off(ctx));
}

void complete(uint64_t hartid, uint32_t source)
{
    int ctx = s_context(hartid);
    if (ctx >= 0) {
        reg(claim_off(ctx)) = source;
    }
}

void dump(uint64_t hartid, uint32_t source)
{
    int ctx = s_context(hartid);
    kprintf("PLIC: source %u priority %u, pending bit %u; context %d enable bit %u, threshold %u\n", source,
            reg(4 * uint64_t{source}), (reg(0x1000 + 4 * (source / 32)) >> (source % 32)) & 1, ctx,
            ctx < 0 ? 0 : (reg(enable_off(ctx, source)) >> (source % 32)) & 1, ctx < 0 ? 0 : reg(threshold_off(ctx)));
}

} // namespace plic
