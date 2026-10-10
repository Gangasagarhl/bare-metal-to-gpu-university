// f408_main.cc - DR302 F4-08: MSI on the edu device, MSI-X on xHCI, then the same
// devices behind Intel VT-d: translated DMA, and a stray DMA caught as a fault.
// Build variants: default (the lab), -DF408_FREE_TOO_EARLY (the forensic lab's driver).
#include "../F4-01/kbase.h"
#include "../F4-02/acpi.h"
#include "intr.h"
#include "msi.h"
#include "dma.h"
#include "vtd.h"
#include "edu.h"
#include "xhci_core.h"

namespace {
volatile uint32_t g_edu_status;                       // last value seen in IRQ_STATUS
uint8_t g_edu_vec, g_xhci_vec, g_fault_vec;
alignas(4096) uint8_t g_src[4096];
alignas(4096) uint8_t g_dst[4096];
alignas(4096) uint8_t g_canary[4096];                 // memory no device was given

void edu_irq(uint8_t)
{
    const uint32_t s = edu::reg(edu::IRQ_STATUS);
    g_edu_status = g_edu_status | s;
    edu::set(edu::IRQ_ACK, s);                        // clear the device's reason
}

void fault_irq(uint8_t)
{
    VtdFault f[4];
    const int n = vtd::read_faults(f, 4);
    for (int i = 0; i < n; ++i)
        kprintf("DMAR fault: source %02x:%02x.%x %s at IOVA 0x%lx, reason %u (%s), t=%u ms\n",
                f[i].source_id >> 8, (f[i].source_id >> 3) & 0x1F, f[i].source_id & 7,
                f[i].read ? "read" : "write", f[i].addr, f[i].reason, vtd::reason_text(f[i].reason),
                static_cast<uint32_t>(clock::ms()));
    for (int i = 0; i < n; ++i)
        kprintf("DMAR fault record (raw): high 0x%016lx low 0x%016lx\n", f[i].raw_hi, f[i].raw_lo);
}

bool wait_status(uint32_t bit, uint32_t ms)
{
    const uint64_t end = clock::ms() + ms;
    while (!(g_edu_status & bit)) {
        if (clock::ms() > end) return false;
        intr::wait();
    }
    g_edu_status = g_edu_status & ~bit;
    return true;
}

void fill(uint8_t* p, uint8_t seed)
{
    for (int i = 0; i < 4096; ++i) p[i] = static_cast<uint8_t>(seed + i * 7);
}

// The edu device copies RAM -> its buffer -> RAM; the driver checks the result.
[[maybe_unused]] bool edu_roundtrip(PciAddr d, const char* label)
{
    fill(g_src, 0x5A);
    memset(g_dst, 0, sizeof g_dst);
    const uint64_t src = dma::map(d, g_src, 4096, false);
    const uint64_t dst = dma::map(d, g_dst, 4096, true);
    kprintf("%s: src buffer phys 0x%x -> IOVA 0x%lx; dst buffer phys 0x%x -> IOVA 0x%lx\n", label,
            reinterpret_cast<uintptr_t>(g_src), src, reinterpret_cast<uintptr_t>(g_dst), dst);
    edu::dma_start(src, 4096, false, true);
    if (!wait_status(edu::IRQ_DMA_DONE, 2000)) return false;
    edu::dma_start(dst, 4096, true, true);
    if (!wait_status(edu::IRQ_DMA_DONE, 2000)) return false;
    const bool ok = memcmp(g_src, g_dst, 4096) == 0;
    kprintf("%s: 4096 bytes RAM -> device -> RAM, %s (fnv1a 0x%08x)\n", label, ok ? "identical" : "DIFFERENT",
            fnv1a(g_dst, 4096));
    dma::unmap(d, src, 4096);
    dma::unmap(d, dst, 4096);
    return ok;
}

// A buggy request: the device is told to write to an address it was never given.
[[maybe_unused]] bool stray_write(PciAddr d)
{
    memset(g_canary, 0xCC, sizeof g_canary);
    fill(g_src, 0x11);
    const uint64_t src = dma::map(d, g_src, 4096, false);
    edu::dma_start(src, 4096, false, true);               // load the device buffer
    if (!wait_status(edu::IRQ_DMA_DONE, 2000)) return false;
    const uint64_t stray = reinterpret_cast<uintptr_t>(g_canary);   // never mapped for d
    kprintf("stray: device told to write 4096 bytes to 0x%lx (the canary's physical address)\n", stray);
    edu::dma_start(stray, 4096, true, true);
    if (!wait_status(edu::IRQ_DMA_DONE, 2000)) return false;
    int changed = 0;
    for (int i = 0; i < 4096; ++i) changed += g_canary[i] != 0xCC;
    kprintf("stray: device reported completion; canary bytes changed: %d of 4096\n", changed);
    dma::unmap(d, src, 4096);
    return true;
}

#ifdef F408_FREE_TOO_EARLY
// Forensic build: the receive path as a colleague wrote it.
void edu_receive_buggy(PciAddr d, int request)
{
    memset(g_dst, 0, sizeof g_dst);
    const uint64_t dst = dma::map(d, g_dst, 4096, true);
    kprintf("edu-rx: request %d mapped rx buffer at IOVA 0x%lx, t=%u ms\n", request, dst,
            static_cast<uint32_t>(clock::ms()));
    edu::dma_start(dst, 4096, true, true);
    dma::unmap(d, dst, 4096);                             // "done with it": released at once
    kprintf("edu-rx: request %d buffer released, t=%u ms\n", request, static_cast<uint32_t>(clock::ms()));
    if (!wait_status(edu::IRQ_DMA_DONE, 2000)) kprintf("edu-rx: request %d timed out\n", request);
    kprintf("edu-rx: request %d complete, t=%u ms, payload fnv1a 0x%08x (expected 0x%08x)\n", request,
            static_cast<uint32_t>(clock::ms()), fnv1a(g_dst, 4096), fnv1a(g_src, 4096));
}
#endif

[[maybe_unused]] void xhci_msix_demo()
{
    PciAddr x;
    if (!xhci::find(x) || !xhci::init(x, g_xhci_vec)) panic("xhci init failed");
    msix::print_entry(xhci::info().msix, 0);
    for (int i = 0; i < 3; ++i) {
        const xhci::Trb ev = xhci::command(xhci::Trb{0, 0, xhci::NOOP_COMMAND << 10});
        kprintf("xhci: no-op command %d -> completion code %u, MSI-X interrupts so far %u (vector 0x%02x count %u)\n",
                i + 1, xhci::completion_code(ev), xhci::interrupts(), g_xhci_vec, intr::count(g_xhci_vec));
    }
    msix::mask(xhci::info().msix, 0, true);
    const uint32_t before = intr::count(g_xhci_vec);
    const xhci::Trb ev = xhci::command(xhci::Trb{0, 0, xhci::NOOP_COMMAND << 10});
    kprintf("xhci: entry 0 masked; no-op completes with code %u, vector count %u -> %u, PBA bit %u\n",
            xhci::completion_code(ev), before, intr::count(g_xhci_vec), msix::pending(xhci::info().msix, 0));
    msix::mask(xhci::info().msix, 0, false);
    clock::sleep_ms(2);
    kprintf("xhci: entry 0 unmasked; vector count now %u, PBA bit %u\n", intr::count(g_xhci_vec),
            msix::pending(xhci::info().msix, 0));
}
}  // namespace

extern "C" void kmain(uint32_t magic, uint32_t)
{
    serial_init();
    kprintf("F4-08 kernel: magic=0x%x\n", magic);
    intr::init();
    intr::print_routing();
    uint64_t ecam;
    uint8_t b0, b1;
    if (acpi::ecam(ecam, b0, b1)) pci::use_ecam(ecam, b0, b1);

    PciAddr d;
    if (!edu::find(d)) panic("no edu device");
    edu::init(d);
    kprintf("edu: %02x:%02x.%x id 0x%08x, liveness(0x12345678) = 0x%08x\n", d.bus, d.dev, d.fn,
            edu::reg(edu::ID), (edu::set(edu::LIVENESS, 0x12345678), edu::reg(edu::LIVENESS)));
    g_edu_vec = intr::alloc_vector();
    g_xhci_vec = intr::alloc_vector();
    g_fault_vec = intr::alloc_vector();
    msi::print(d);
    intr::set_handler(g_edu_vec, edu_irq);
    if (!msi::enable(d, g_edu_vec)) panic("edu: no MSI");
    msi::print(d);

    const bool iommu = dma::use_iommu();
    if (iommu) {
        PciAddr x;
        dma::attach(d);
        if (xhci::find(x)) dma::attach(x);
        intr::set_handler(g_fault_vec, fault_irq);
        vtd::fault_irq(g_fault_vec);
        vtd::enable();
    } else {
        dma::use_identity();
        kprintf("dma: identity mappings (IOVA = physical address)\n");
    }

    for (uint32_t i = 1; i <= 5; ++i) edu::set(edu::IRQ_RAISE, i << 4);
    clock::sleep_ms(5);
    kprintf("edu: 5 raise requests -> %u interrupts on vector 0x%02x, status bits seen 0x%x\n",
            intr::count(g_edu_vec), g_edu_vec, g_edu_status);
    g_edu_status = 0;
    edu::set(edu::STATUS, edu::STATUS_IRQ_FACT);
    edu::set(edu::FACTORIAL, 10);
    if (!wait_status(0x1, 2000)) panic("edu: no factorial interrupt");
    kprintf("edu: factorial(10) = %u (interrupt after the computation)\n", edu::reg(edu::FACTORIAL));

#ifdef F408_FREE_TOO_EARLY
    fill(g_src, 0x77);
    const uint64_t s = dma::map(d, g_src, 4096, false);
    edu::dma_start(s, 4096, false, true);                 // the device holds a payload
    wait_status(edu::IRQ_DMA_DONE, 2000);
    dma::unmap(d, s, 4096);
    for (int r = 1; r <= 2; ++r) edu_receive_buggy(d, r);
    clock::sleep_ms(20);
#else
    if (!edu_roundtrip(d, iommu ? "dma(iommu)" : "dma(identity)")) panic("edu round trip failed");
    if (iommu) {
        const uint64_t iova = dma::map(d, g_dst, 4096, true);
        vtd::dump_walk(d, iova);
        vtd::dump_walk(d, iova + 4096);
        dma::unmap(d, iova, 4096);
    }
    if (!stray_write(d)) panic("stray test: no completion");
    clock::sleep_ms(20);
    xhci_msix_demo();
#endif
    kprintf("interrupt counts: tick(0x20)=%s edu(0x%02x)=%u xhci(0x%02x)=%u dmar-fault(0x%02x)=%u\n",
            clock::ms() > 0 ? "running" : "stopped", g_edu_vec, intr::count(g_edu_vec), g_xhci_vec,
            intr::count(g_xhci_vec), g_fault_vec, intr::count(g_fault_vec));
    kprintf("F4-08 done\n");
    qemu_exit(0x10);
}
