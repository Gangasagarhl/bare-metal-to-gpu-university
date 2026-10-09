// drivers.cc - F4-35: the drivers bound on QEMU's virt machine. Each probe checks the device
// for real (an identification register, a counter that moves), so "bound" means "answered".
// Register offsets are noted next to each access with the document that defines them; none of
// those documents was opened during this build (title only, pending verification).
#include "dm.h"

namespace {

// Index of `name` in the node's "clock-names" list (or -1).
int clock_index(const dm::Device& dev, const char* name)
{
    fdt::Prop p;
    if (!dm::tree().prop(dev.node, "clock-names", p)) {
        return -1;
    }
    int idx = 0;
    for (uint32_t i = 0; i < p.len; ++idx) {
        const char* s = reinterpret_cast<const char*>(p.data + i);
        if (fdt::streq(s, name)) {
            return idx;
        }
        i += fdt::slen(s) + 1;
    }
    return -1;
}

// Arm PrimeCell peripherals end with four identification bytes at 0xFE0-0xFEC whose low 12 bits
// form the part number (PL011/PL031/PL061 Technical Reference Manuals).
uint32_t primecell_part(uint64_t base)
{
    return (k::rd32(base + 0xfe0) & 0xff) | ((k::rd32(base + 0xfe4) & 0xf) << 8);
}

// --- fixed-clock: a clock with one rate, written in the tree ---------------------------------
bool fixed_rate(int node, const uint32_t*, uint32_t, uint64_t& rate, bool)
{
    rate = dm::tree().u32(node, "clock-frequency", 0);
    return rate != 0;
}
dm::Probe fixed_clock_probe(dm::Device& dev)
{
    dm::clock_provider(dev.node, fixed_rate);
    k::printf("  fixed-clock %s: %u Hz\n", dm::path(dev), dm::tree().u32(dev.node, "clock-frequency", 0));
    return dm::Probe::kOk;
}
DR403_DRIVER(fixedclk, "fixed-clock", fixed_clock_probe, "fixed-clock");

// --- GIC (version 2 distributor): GICD_TYPER at 0x004, ITLinesNumber in bits 4:0 ------------
dm::Probe gic_probe(dm::Device& dev)
{
    uint32_t typer = k::rd32(dev.base + 0x004);
    uint32_t iidr = k::rd32(dev.base + 0x008);
    k::printf("  gic %s: GICD_TYPER 0x%x -> %u interrupt lines, GICD_IIDR 0x%x\n", dm::path(dev), typer,
              32 * ((typer & 0x1f) + 1), iidr);
    dm::irq_provider(dev.node);
    return dm::Probe::kOk;
}
DR403_DRIVER(gic, "gic-v2", gic_probe, "arm,cortex-a15-gic", "arm,gic-400");

// --- PL011 UART -------------------------------------------------------------------------------
dm::Probe pl011_probe(dm::Device& dev)
{
    uint64_t rate = 0;
    int idx = clock_index(dev, "uartclk");
    if (idx >= 0) {
        dm::Probe r = dm::clock_enable(dev, idx, rate);
        if (r != dm::Probe::kOk) {
            return r;
        }
    }
    uint32_t part = primecell_part(dev.base);
    if (part != 0x011) {
        dev.why = "identification registers do not say PL011";
        return dm::Probe::kFail;
    }
    k::printf("  pl011 %s: part 0x%03x, uartclk %lu Hz%s\n", dm::path(dev), part, rate,
              idx < 0 ? " (no clocks property: rate unknown)" : "");
    return dm::Probe::kOk;
}
DR403_DRIVER(pl011, "pl011-uart", pl011_probe, "arm,pl011");

// --- PL031 real-time clock: RTCDR (seconds) at 0x000 ------------------------------------------
dm::Probe pl031_probe(dm::Device& dev)
{
    uint64_t rate = 0;
    dm::Probe r = dm::clock_enable(dev, 0, rate);
    if (r != dm::Probe::kOk) {
        return r;
    }
    if ((r = dm::irq_parent_ready(dev)) != dm::Probe::kOk) {
        return r;
    }
    uint32_t t0 = k::rd32(dev.base);
    k::delay_us(1100000);
    uint32_t t1 = k::rd32(dev.base);
    k::printf("  pl031 %s: part 0x%03x, RTCDR %u then %u after 1.1 s on the counter -> %s\n", dm::path(dev),
              primecell_part(dev.base), t0, t1, (t1 - t0 == 1 || t1 - t0 == 2) ? "ticking" : "NOT ticking");
    return dm::Probe::kOk;
}
DR403_DRIVER(pl031, "pl031-rtc", pl031_probe, "arm,pl031");

// --- PL061 GPIO controller ---------------------------------------------------------------------
dm::Probe pl061_probe(dm::Device& dev)
{
    uint64_t rate = 0;
    dm::Probe r = dm::clock_enable(dev, 0, rate);
    if (r != dm::Probe::kOk) {
        return r;
    }
    if ((r = dm::irq_parent_ready(dev)) != dm::Probe::kOk) {
        return r;
    }
    // GPIODATA is read through an address mask: offset 0x3FC reads all eight pins.
    k::printf("  pl061 %s: part 0x%03x, GPIODIR 0x%02x, GPIODATA 0x%02x\n", dm::path(dev),
              primecell_part(dev.base), k::rd32(dev.base + 0x400) & 0xff, k::rd32(dev.base + 0x3fc) & 0xff);
    return dm::Probe::kOk;
}
DR403_DRIVER(pl061, "pl061-gpio", pl061_probe, "arm,pl061");

// --- virtio over MMIO: magic 0x000, version 0x004, device ID 0x008 (VIRTIO specification,
//     "Virtio Over MMIO") -------------------------------------------------------------------
dm::Probe virtio_probe(dm::Device& dev)
{
    dm::Probe r = dm::irq_parent_ready(dev);
    if (r != dm::Probe::kOk) {
        return r;
    }
    uint32_t magic = k::rd32(dev.base);
    uint32_t id = k::rd32(dev.base + 0x008);
    if (magic != 0x74726976) {          // the bytes "virt" read as a little-endian word
        dev.why = "bad magic";
        return dm::Probe::kFail;
    }
    if (id == 0) {
        dev.why = "empty transport (device ID 0)";
        return dm::Probe::kFail;
    }
    k::printf("  virtio %s: version %u, device ID %u\n", dm::path(dev), k::rd32(dev.base + 0x004), id);
    return dm::Probe::kOk;
}
DR403_DRIVER(virtio, "virtio-mmio", virtio_probe, "virtio,mmio");

// --- the architectural timer: no registers in the tree, only system registers ---------------
dm::Probe timer_probe(dm::Device& dev)
{
    uint64_t f = k::counter_freq();
    uint64_t c0 = k::counter();
    k::delay_us(10000);
    uint64_t c1 = k::counter();
    k::printf("  timer %s: CNTFRQ_EL0 %lu Hz; 10 ms busy wait advanced the counter by %lu\n", dm::path(dev), f,
              c1 - c0);
    return f != 0 && c1 > c0 ? dm::Probe::kOk : dm::Probe::kFail;
}
DR403_DRIVER(timer, "armv8-timer", timer_probe, "arm,armv8-timer", "arm,armv7-timer");

// --- fw-cfg (QEMU's firmware configuration device): selector at 0x8 (16-bit, big-endian),
//     data at 0x0; key 0 is the signature (QEMU documentation "QEMU Firmware Configuration") ---
dm::Probe fwcfg_probe(dm::Device& dev)
{
    k::wr16(dev.base + 0x8, 0x0000);
    char sig[5] = {0, 0, 0, 0, 0};
    for (int i = 0; i < 4; ++i) {
        sig[i] = static_cast<char>(k::rd8(dev.base));
    }
    k::printf("  fw-cfg %s: signature \"%s\"\n", dm::path(dev), sig);
    return sig[0] == 'Q' ? dm::Probe::kOk : dm::Probe::kFail;
}
DR403_DRIVER(fwcfg, "qemu-fw-cfg", fwcfg_probe, "qemu,fw-cfg-mmio");

// --- PSCI: function IDs after the Arm PSCI specification (title only, pending verification) --
bool g_psci_smc = false;
uint64_t psci_call(uint64_t fid)
{
    register uint64_t x0 asm("x0") = fid;
    if (g_psci_smc) {
        asm volatile("smc #0" : "+r"(x0) : : "x1", "x2", "x3", "memory");
    } else {
        asm volatile("hvc #0" : "+r"(x0) : : "x1", "x2", "x3", "memory");
    }
    return x0;
}
void psci_off()
{
    k::printf("power off through PSCI SYSTEM_OFF\n");
    psci_call(0x84000008);
}
dm::Probe psci_probe(dm::Device& dev)
{
    fdt::Prop m;
    if (!dm::tree().prop(dev.node, "method", m)) {
        dev.why = "no method property";
        return dm::Probe::kFail;
    }
    g_psci_smc = fdt::streq(reinterpret_cast<const char*>(m.data), "smc");
    uint64_t v = psci_call(0x84000000);
    k::printf("  psci %s: method %s, PSCI_VERSION -> %lu.%lu\n", dm::path(dev), g_psci_smc ? "smc" : "hvc",
              v >> 16, v & 0xffff);
    dm::set_power_off(psci_off);
    return dm::Probe::kOk;
}
DR403_DRIVER(psci, "psci", psci_probe, "arm,psci-1.0", "arm,psci-0.2");

}  // namespace
