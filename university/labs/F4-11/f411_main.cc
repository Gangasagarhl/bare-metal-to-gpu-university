// f411_main.cc - DR302 F4-11 lab kernel (milestone C10): ACPI without an interpreter.
//   1. FADT: where the fixed ACPI hardware lives; switch to ACPI mode; PM timer check
//   2. DSDT: walk the AML namespace, read \_S5 (the soft-off sleep type)
//   3. Routing: the e1000's INTA# through the static _PRT table and a GSI link device,
//      then prove the route by making the NIC raise its interrupt line
//   4. The power button: SCI -> PWRBTN_STS -> shut the machine down through PM1a_CNT
// The forensic build (-DF411_BYTE_WRITE) writes PM1a_CNT with a byte access.
#include "kbase.h"
#include "../F4-02/acpi.h"
#include "../F4-02/pci.h"
#include "../F4-08/intr.h"
#include "aml.h"
#include "pm.h"

namespace {
pm::Fadt g_f;
const uint8_t* g_dsdt;
uint32_t g_dsdt_len;
volatile uint32_t g_pm1_seen, g_pwrbtn;
volatile uint64_t g_pwrbtn_ms;
uintptr_t g_nic;
volatile uint32_t g_icr_seen;

// The SCI is level-triggered and shared by every ACPI event source: read the status,
// clear exactly the bits handled (write 1), and let the dispatcher send the EOI after.
void sci_irq(uint8_t)
{
    const uint16_t s = pm::status(g_f);
    g_pm1_seen = g_pm1_seen | s;
    if (s & pm1::PWRBTN_STS) {
        pm::clear_status(g_f, pm1::PWRBTN_STS);
        if (!g_pwrbtn) g_pwrbtn_ms = clock::ms();
        g_pwrbtn = g_pwrbtn + 1;
    }
}

// e1000 legacy interrupt: reading ICR returns the causes and clears them, which
// deasserts INTA#. Without the read the level stays high and the handler runs again.
constexpr uint32_t E1000_ICR = 0xC0, E1000_ICS = 0xC8, E1000_IMS = 0xD0, E1000_IMC = 0xD8, ICR_LSC = 1u << 2;
void nic_irq(uint8_t)
{
    g_icr_seen = g_icr_seen | mmio_read<uint32_t>(g_nic + E1000_ICR);
}

PciAddr g_e1000;
bool g_e1000_found;
void probe(PciAddr a)
{
    if (pci::read16(a, pcireg::VENDOR_ID) == 0x8086 && pci::read16(a, pcireg::DEVICE_ID) == 0x100E) {
        g_e1000 = a;
        g_e1000_found = true;
    }
}

struct Count { uint32_t devices, methods, names; };
void count_visit(const aml::Node& n, void* ctx)
{
    auto* c = static_cast<Count*>(ctx);
    if (n.kind == aml::Kind::Device) {
        ++c->devices;
        kprintf("aml: device %s\n", n.path);          // the namespace device list (milestone C10)
    }
    if (n.kind == aml::Kind::Method) ++c->methods;
    if (n.kind == aml::Kind::Name) ++c->names;
}

bool read_name(const char* path, aml::Value& v)
{
    aml::Node n;
    if (!aml::find(g_dsdt, g_dsdt_len, path, n) || n.kind != aml::Kind::Name) return false;
    const uint8_t* p = n.obj;
    return aml::value(p, n.end, v);
}

// Looks up (device, pin) in \_SB.PCI0.PRTA and follows the link device's _PRS.
bool route_pci(uint8_t dev, uint8_t pin, uint32_t& gsi, bool& level, bool& low, char* link, uint32_t cap)
{
    aml::Value table, entry, e;
    if (!read_name("\\_SB.PCI0.PRTA", table) || table.t != aml::T::Package) return false;
    const uint8_t* q = table.p;
    for (uint32_t i = 0; i < table.n && aml::value(q, table.end, entry); ++i) {
        const uint8_t* r = entry.p;
        uint64_t addr = 0, epin = 0;
        char src[96] = "";
        for (uint32_t k = 0; k < 4 && aml::value(r, entry.end, e); ++k) {
            if (k == 0) addr = e.i;
            if (k == 1) epin = e.i;
            if (k == 2 && e.t == aml::T::NameRef) memcpy(src, e.name, sizeof src);
        }
        if ((addr >> 16) != dev || epin != pin) continue;
        // The source is a name relative to \_SB.PCI0; QEMU's link devices live in \_SB.
        // (Real name resolution searches upward through the parent scopes.)
        char path[64] = "\\_SB.";
        uint32_t n = 5;
        for (uint32_t k = 0; src[k] && n + 6 < sizeof path; ++k) path[n++] = src[k];
        memcpy(path + n, "._PRS", 6);
        for (uint32_t k = 0; k + 1 < cap && src[k]; ++k) { link[k] = src[k]; link[k + 1] = 0; }
        aml::Value prs;
        uint8_t flags;
        if (!read_name(path, prs) || !aml::first_interrupt(prs, gsi, flags)) return false;
        kprintf("route: PRTA entry %u: address 0x%08x pin %u -> %s; %s offers interrupt %u (%s, active %s)\n", i,
                static_cast<uint32_t>(addr), static_cast<uint32_t>(epin), src, path, gsi, (flags & 2) ? "edge" : "level",
                (flags & 4) ? "low" : "high");
        level = !(flags & 2);
        low = (flags & 4) != 0;
        return true;
    }
    return false;
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-11 kernel: magic=0x%x\n", magic);
    intr::init();
    uint64_t ecam;
    uint8_t b0, b1;
    if (acpi::ecam(ecam, b0, b1)) pci::use_ecam(ecam, b0, b1);

    // 1. FADT and ACPI mode.
    if (!pm::read_fadt(g_f)) panic("no FADT");
    pm::print(g_f);
    if (!pm::enable_acpi_mode(g_f)) panic("could not enter ACPI mode");
    const uint32_t t0 = pm::timer(g_f);
    const uint64_t m0 = clock::ms();
    clock::sleep_ms(200);
    const uint32_t t1 = pm::timer(g_f);
    const uint64_t m1 = clock::ms();
    const uint32_t mask = g_f.tmr32 ? 0xFFFFFFFFu : 0xFFFFFFu;
    const uint32_t ticks = (t1 - t0) & mask;
    kprintf("pmtimer: %u ticks in %u ms of the PIT clock -> %u ticks per second (nominal %u)\n", ticks,
            static_cast<uint32_t>(m1 - m0), static_cast<uint32_t>(uint64_t{ticks} * 1000 / (m1 - m0)), pm1::PM_TIMER_HZ);

    // 2. The DSDT and \_S5.
    const auto* h = reinterpret_cast<const AcpiHeader*>(g_f.dsdt);
    g_dsdt = reinterpret_cast<const uint8_t*>(g_f.dsdt);
    g_dsdt_len = h->length;
    uint8_t sum = 0;
    for (uint32_t i = 0; i < g_dsdt_len; ++i) sum = static_cast<uint8_t>(sum + g_dsdt[i]);
    kprintf("dsdt: signature %c%c%c%c, %u bytes, checksum %s\n", h->signature[0], h->signature[1], h->signature[2],
            h->signature[3], g_dsdt_len, sum == 0 ? "ok" : "BAD");
#ifdef F411_DUMP
    for (uint32_t i = 0; i < g_dsdt_len; i += 32) {
        kprintf("dsdtdump:");
        for (uint32_t j = i; j < i + 32 && j < g_dsdt_len; ++j) kprintf("%02x", g_dsdt[j]);
        kprintf("\n");
    }
    qemu_exit(0x10);
#endif
    Count cnt{0, 0, 0};
    const aml::Result r = aml::walk(g_dsdt, g_dsdt_len, count_visit, &cnt);
    kprintf("aml: %u named objects (%u devices, %u methods, %u names), walk %s\n", r.nodes, cnt.devices, cnt.methods,
            cnt.names, r.complete ? "complete" : "STOPPED");
    aml::Value s5, e;
    uint8_t slp_a = 0, slp_b = 0;
    if (!read_name("\\_S5", s5) || s5.t != aml::T::Package) panic("no static \\_S5 package");
    const uint8_t* q = s5.p;
    if (aml::value(q, s5.end, e) && e.t == aml::T::Int) slp_a = static_cast<uint8_t>(e.i);
    if (aml::value(q, s5.end, e) && e.t == aml::T::Int) slp_b = static_cast<uint8_t>(e.i);
    kprintf("aml: \\_S5 = Package(%u) { SLP_TYPa %u, SLP_TYPb %u, ... }\n", s5.n, slp_a, slp_b);

    // 3. Routing the e1000's legacy interrupt through the ACPI tables.
    pci::enumerate(probe);
    if (!g_e1000_found) panic("no e1000");
    const uint8_t pin = pci::read8(g_e1000, pcireg::INTERRUPT_PIN);
    const uint8_t line = pci::read8(g_e1000, pcireg::INTERRUPT_LINE);
    kprintf("route: e1000 at %02x:%02x.%x, interrupt pin %u (INT%c#), interrupt line register %u (written by the BIOS)\n",
            g_e1000.bus, g_e1000.dev, g_e1000.fn, pin, 'A' + pin - 1, line);
    uint32_t gsi = 0;
    bool level = true, low = false;
    char link[8] = "";
    if (!route_pci(g_e1000.dev, static_cast<uint8_t>(pin - 1), gsi, level, low, link, sizeof link)) panic("no _PRT route");
    Bar bars[6];
    pci::size_bars(g_e1000, bars, 6);
    g_nic = static_cast<uintptr_t>(bars[0].addr);
    pci::enable(g_e1000, pcireg::CMD_MEMORY);
    const uint16_t cmd = pci::read16(g_e1000, pcireg::COMMAND);
    pci::write16(g_e1000, pcireg::COMMAND, static_cast<uint16_t>(cmd & ~pcireg::CMD_INTX_DISABLE));
    const uint8_t nic_vec = intr::alloc_vector();
    intr::set_handler(nic_vec, nic_irq);
    intr::route_gsi(gsi, nic_vec, level, low);
    mmio_write<uint32_t>(g_nic + E1000_IMC, 0xFFFFFFFFu);
    (void)mmio_read<uint32_t>(g_nic + E1000_ICR);
    mmio_write<uint32_t>(g_nic + E1000_IMS, ICR_LSC);
    for (int i = 0; i < 3; ++i) {
        mmio_write<uint32_t>(g_nic + E1000_ICS, ICR_LSC);    // "interrupt cause set": the NIC raises INTA#
        clock::sleep_ms(2);
    }
    mmio_write<uint32_t>(g_nic + E1000_IMC, 0xFFFFFFFFu);
    kprintf("route: 3 interrupt-cause writes -> %u interrupts on vector 0x%02x (GSI %u via %s), ICR bits seen 0x%08x\n",
            intr::count(nic_vec), nic_vec, gsi, link, g_icr_seen);
    const bool route_ok = intr::count(nic_vec) == 3;

    // 4. The power button.
    bool sci_level, sci_low;
    const uint32_t sci_gsi = intr::isa_to_gsi(static_cast<uint8_t>(g_f.sci_irq), &sci_level, &sci_low);
    const uint8_t sci_vec = intr::alloc_vector();
    intr::set_handler(sci_vec, sci_irq);
    pm::clear_status(g_f, pm1::PWRBTN_STS);
    intr::route_gsi(sci_gsi, sci_vec, sci_level, sci_low);
    pm::set_enable(g_f, pm1::PWRBTN_EN);
    kprintf("sci: ISA IRQ %u -> GSI %u (%s, active %s) -> vector 0x%02x; PWRBTN_EN set\n", g_f.sci_irq, sci_gsi,
            sci_level ? "level" : "edge", sci_low ? "low" : "high", sci_vec);
    intr::print_routing();
    kprintf("acpi: press the power button (t=%u ms)\n", static_cast<uint32_t>(clock::ms()));
    const uint64_t end = clock::ms() + 15000;
    while (!g_pwrbtn && clock::ms() < end) intr::wait();
    if (!g_pwrbtn) {
        kprintf("acpi: no power button event in 15 s (PM1 status 0x%04x)\n", pm::status(g_f));
        kprintf("F4-11 FAILED\n");
        qemu_exit(1);
    }
    kprintf("sci: power button at t=%u ms: %u SCI(s), PM1 status bits seen 0x%04x, now 0x%04x\n",
            static_cast<uint32_t>(g_pwrbtn_ms), intr::count(sci_vec), g_pm1_seen, pm::status(g_f));
    if (!route_ok) {
        kprintf("F4-11 FAILED (routing)\n");
        qemu_exit(1);
    }
    kprintf("acpi: shutting down (S5, SLP_TYPa %u)\n", slp_a);
    pm::sleep(g_f, slp_a, slp_b);
    clock::sleep_ms(2000);
    kprintf("acpi: still running 2000 ms after the S5 request; PM1a_CNT reads 0x%04x\n", pm::control(g_f));
    kprintf("F4-11 FAILED\n");
    qemu_exit(1);
}
