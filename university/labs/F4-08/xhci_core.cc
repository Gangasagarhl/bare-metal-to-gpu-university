// xhci_core.cc - DR302 F4-08: xHCI reset, command and event rings, interrupter 0.
#include "xhci_core.h"
#include "dma.h"
#include "intr.h"
#include "../F4-01/kbase.h"

namespace {
using namespace xhci;
// Operational registers (from op base)
constexpr uint32_t USBCMD = 0x00, USBSTS = 0x04, CRCR = 0x18, DCBAAP = 0x30, CONFIG = 0x38;
constexpr uint32_t CMD_RS = 1u << 0, CMD_HCRST = 1u << 1, CMD_INTE = 1u << 2;
constexpr uint32_t STS_HCH = 1u << 0, STS_EINT = 1u << 3, STS_CNR = 1u << 11;
// Interrupter 0 registers (from runtime base + 0x20)
constexpr uint32_t IR0 = 0x20, IMAN = 0x00, ERSTSZ = 0x08, ERSTBA = 0x10, ERDP = 0x18;
constexpr uint32_t IMAN_IP = 1u << 0, IMAN_IE = 1u << 1;
constexpr uint64_t ERDP_EHB = 1u << 3;

Info g;
alignas(4096) Trb g_cmd_mem[32];
alignas(4096) Trb g_evt_mem[64];
struct alignas(64) ErstEntry { uint64_t base; uint32_t size; uint32_t rsvd; };
alignas(64) ErstEntry g_erst[1];
alignas(4096) uint64_t g_dcbaa[256];
alignas(4096) uint8_t g_scratch_arr[4096];
alignas(4096) uint8_t g_scratch_pages[8][4096];
Ring g_cmd;
uint32_t g_evt_deq = 0, g_evt_cycle = 1;
uint64_t g_evt_iova = 0;
volatile uint32_t g_irqs = 0;
Trb g_pending[16];                                // events seen while waiting for a command
int g_npending = 0;
PciAddr g_found{0xFF, 0, 0};

uint32_t op32(uint32_t off) { return mmio_read<uint32_t>(g.op + off); }
void op32w(uint32_t off, uint32_t v) { mmio_write<uint32_t>(g.op + off, v); }
void op64w(uint32_t off, uint64_t v)
{
    mmio_write<uint32_t>(g.op + off, static_cast<uint32_t>(v));
    mmio_write<uint32_t>(g.op + off + 4, static_cast<uint32_t>(v >> 32));
}
uintptr_t ir0(uint32_t off) { return g.rt + IR0 + off; }

bool wait_bits(uint32_t off, uint32_t mask, uint32_t want, uint32_t ms)
{
    const uint64_t end = clock::ms() + ms;
    while ((op32(off) & mask) != want)
        if (clock::ms() > end) return false;
    return true;
}

void on_interrupt(uint8_t)
{
    g_irqs = g_irqs + 1;
    // Acknowledge at the controller: clear USBSTS.EINT and IMAN.IP (both write-1-to-clear).
    op32w(USBSTS, STS_EINT);
    mmio_write<uint32_t>(ir0(IMAN), IMAN_IE | IMAN_IP);
}

// Takes the next event from the event ring if its cycle bit says the controller wrote it.
bool pop_event(Trb& ev)
{
    const Trb& t = g_evt_mem[g_evt_deq];
    compiler_barrier();
    if ((t.control & 1) != g_evt_cycle) return false;
    ev = t;
    if (++g_evt_deq == 64) { g_evt_deq = 0; g_evt_cycle ^= 1; }
    const uint64_t erdp = g_evt_iova + 16ull * g_evt_deq;
    mmio_write<uint32_t>(ir0(ERDP), static_cast<uint32_t>(erdp | ERDP_EHB));
    mmio_write<uint32_t>(ir0(ERDP) + 4, static_cast<uint32_t>(erdp >> 32));
    return true;
}

void probe(PciAddr a)
{
    if ((pci::read32(a, pcireg::CLASS_REVISION) >> 8) == 0x0C0330) g_found = a;
}
}  // namespace

namespace xhci {
void ring_init(Ring& r, Trb* mem, uint32_t n)
{
    memset(mem, 0, n * sizeof(Trb));
    r.trb = mem;
    r.n = n;
    r.enq = 0;
    r.cycle = 1;
    r.iova = dma::map(g.pci, mem, n * sizeof(Trb), false);
    mem[n - 1].param = r.iova;                               // Link TRB back to the start
    mem[n - 1].control = (LINK << 10) | (1u << 1);           // bit 1: toggle cycle
}

uint64_t ring_push(Ring& r, Trb t)
{
    t.control = (t.control & ~1u) | r.cycle;
    Trb& slot = r.trb[r.enq];
    slot.param = t.param;
    slot.status = t.status;
    compiler_barrier();
    slot.control = t.control;                                // cycle bit written last
    const uint64_t at = r.iova + 16ull * r.enq;
    if (++r.enq == r.n - 1) {                                // reached the Link TRB
        Trb& link = r.trb[r.n - 1];
        link.control = (link.control & ~1u) | r.cycle;       // hand it to the controller
        r.enq = 0;
        r.cycle ^= 1;
    }
    return at;
}

const Info& info() { return g; }
void* dcbaa() { return g_dcbaa; }
uint32_t interrupts() { return g_irqs; }
uint64_t iova_of(const void* p, uint32_t bytes, bool w) { return dma::map(g.pci, p, bytes, w); }

bool find(PciAddr& out)
{
    pci::enumerate(probe);
    out = g_found;
    return g_found.bus != 0xFF;
}

bool init(PciAddr a, uint8_t vector)
{
    g.pci = a;
    g.vector = vector;
    Bar bars[6];
    pci::size_bars(a, bars, 6);
    g.mmio = static_cast<uintptr_t>(bars[0].addr);
    pci::enable(a, pcireg::CMD_MEMORY | pcireg::CMD_MASTER);
    const uint32_t cap0 = mmio_read<uint32_t>(g.mmio);
    const uint32_t hcs1 = mmio_read<uint32_t>(g.mmio + 0x04), hcs2 = mmio_read<uint32_t>(g.mmio + 0x08);
    const uint32_t hcc1 = mmio_read<uint32_t>(g.mmio + 0x10);
    g.op = g.mmio + (cap0 & 0xFF);
    g.db = g.mmio + (mmio_read<uint32_t>(g.mmio + 0x14) & ~0x3u);
    g.rt = g.mmio + (mmio_read<uint32_t>(g.mmio + 0x18) & ~0x1Fu);
    g.version = static_cast<uint16_t>(cap0 >> 16);
    g.max_slots = hcs1 & 0xFF;
    g.max_intrs = (hcs1 >> 8) & 0x7FF;
    g.max_ports = static_cast<uint8_t>(hcs1 >> 24);
    g.context_size = (hcc1 & (1u << 2)) ? 64 : 32;
    g.scratchpads = (((hcs2 >> 21) & 0x1F) << 5) | ((hcs2 >> 27) & 0x1F);
    kprintf("xhci: %02x:%02x.%x version %x.%02x, %u slots, %u interrupters, %u ports, "
            "%u-byte contexts, %u scratchpad pages, 64-bit addressing %u\n", a.bus, a.dev, a.fn,
            g.version >> 8, g.version & 0xFF, g.max_slots, g.max_intrs, g.max_ports, g.context_size,
            g.scratchpads, hcc1 & 1);

    if (!wait_bits(USBSTS, STS_CNR, 0, 1000)) return false;
    op32w(USBCMD, op32(USBCMD) & ~CMD_RS);
    if (!wait_bits(USBSTS, STS_HCH, STS_HCH, 1000)) return false;
    op32w(USBCMD, CMD_HCRST);
    if (!wait_bits(USBCMD, CMD_HCRST, 0, 1000) || !wait_bits(USBSTS, STS_CNR, 0, 1000)) return false;

    dma::attach(a);
    op32w(CONFIG, g.max_slots);
    memset(g_dcbaa, 0, sizeof g_dcbaa);
    if (g.scratchpads) {
        if (g.scratchpads > 8) return false;
        auto* arr = reinterpret_cast<uint64_t*>(g_scratch_arr);
        for (uint32_t i = 0; i < g.scratchpads; ++i) arr[i] = dma::map(a, g_scratch_pages[i], 4096, true);
        g_dcbaa[0] = dma::map(a, g_scratch_arr, 4096, false);
    }
    op64w(DCBAAP, dma::map(a, g_dcbaa, sizeof g_dcbaa, true));
    ring_init(g_cmd, g_cmd_mem, 32);
    op64w(CRCR, g_cmd.iova | 1);                            // RCS = 1
    memset(g_evt_mem, 0, sizeof g_evt_mem);
    g_evt_iova = dma::map(a, g_evt_mem, sizeof g_evt_mem, true);
    g_erst[0] = ErstEntry{g_evt_iova, 64, 0};
    mmio_write<uint32_t>(ir0(ERSTSZ), 1);
    mmio_write<uint32_t>(ir0(ERDP), static_cast<uint32_t>(g_evt_iova));
    mmio_write<uint32_t>(ir0(ERDP) + 4, static_cast<uint32_t>(g_evt_iova >> 32));
    const uint64_t erst = dma::map(a, g_erst, sizeof g_erst, false);
    mmio_write<uint32_t>(ir0(ERSTBA), static_cast<uint32_t>(erst));   // written last: the
    mmio_write<uint32_t>(ir0(ERSTBA) + 4, static_cast<uint32_t>(erst >> 32));   // ring goes live
    if (vector) {
        if (!msix::setup(a, g.msix)) return false;
        msix::program(g.msix, 0, vector);
        intr::set_handler(vector, on_interrupt);
        msix::mask(g.msix, 0, false);
        mmio_write<uint32_t>(ir0(IMAN), IMAN_IE);
    }
    op32w(USBCMD, CMD_RS | (vector ? CMD_INTE : 0));
    return wait_bits(USBSTS, STS_HCH, 0, 1000);
}

void doorbell(uint8_t slot, uint32_t target) { mmio_write<uint32_t>(g.db + 4u * slot, target); }

Trb command(Trb cmd, uint32_t timeout_ms)
{
    const uint64_t at = ring_push(g_cmd, cmd);
    doorbell(0, 0);
    const uint64_t end = clock::ms() + timeout_ms;
    for (;;) {
        Trb ev;
        while (pop_event(ev)) {
            if (trb_type(ev) == COMMAND_COMPLETION && ev.param == at) return ev;
            if (g_npending < 16) g_pending[g_npending++] = ev;
        }
        if (clock::ms() > end) return Trb{0, 0, 0};
        if (g.vector) intr::wait(); // sleep until the MSI-X (or the 1 kHz tick) arrives
    }
}

bool wait_event(Trb& ev, uint32_t timeout_ms)
{
    if (g_npending) {
        ev = g_pending[0];
        for (int i = 1; i < g_npending; ++i) g_pending[i - 1] = g_pending[i];
        --g_npending;
        return true;
    }
    const uint64_t end = clock::ms() + timeout_ms;
    for (;;) {
        if (pop_event(ev)) return true;
        if (clock::ms() > end) return false;
        if (g.vector) intr::wait();
    }
}

uint32_t portsc(uint8_t port) { return mmio_read<uint32_t>(g.op + 0x400 + 0x10u * (port - 1)); }
void set_portsc(uint8_t port, uint32_t v) { mmio_write<uint32_t>(g.op + 0x400 + 0x10u * (port - 1), v); }
}  // namespace xhci
