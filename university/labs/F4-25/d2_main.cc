// d2_main.cc - F4-25: milestone D2's arch pieces on QEMU virt: translation tables with W^X,
// GICv3, the generic timer as a scheduler tick, and a device interrupt (PL011 receive) as an SPI.
#include <atomic>
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "gic.h"
#include "gtimer.h"
#include "kprint.h"
#include "mmu.h"
#include "neutral_tests.h"
#include "pl011.h"
#include "psci.h"
#include "trap.h"

namespace {

// Shared between the interrupt handler and kmain: atomics, so the compiler re-reads them.
std::atomic<uint64_t> g_ticks{0};
std::atomic<uint32_t> g_rx{0};
char g_rx_buf[32];
uint64_t g_interval = 0;
uint32_t g_timer_intid = 0, g_uart_intid = 0;
uint32_t g_ret_insn[1] = {0xd65f03c0};   // "ret", but in a data page: must not be executable

void irq_handler(TrapFrame*)
{
    uint32_t id = gic::ack();
    if (id == g_timer_intid) {
#ifndef FORENSIC_NO_REARM
        gtimer::arm(g_interval);          // set the next deadline: the line drops again
#endif
        uint64_t t = g_ticks.fetch_add(1, std::memory_order_relaxed) + 1;
        if (t % 100000 == 0) {
            kprintf("irq: tick %lu at counter %lu, CNTP_CTL_EL0 0x%lx\n", t, gtimer::now(), gtimer::ctl());
        }
    } else if (id == g_uart_intid) {
        char c;
        while (pl011::read_char(&c)) {    // reading empties the FIFO, which drops the line
            uint32_t n = g_rx.load(std::memory_order_relaxed);
            if (n + 1 < sizeof g_rx_buf) {
                g_rx_buf[n] = c;
                g_rx.store(n + 1, std::memory_order_release);
            }
        }
    } else if (id == gic::kSpurious) {
        return;                           // nothing to acknowledge
    } else {
        kprintf("irq: unexpected INTID %u\n", id);
    }
    gic::eoi(id);
}

bool console(const fdt::Blob& dt)
{
    fdt::Node uart;
    uint64_t base = 0, size = 0;
    if (!dt.find_compatible("arm,pl011", &uart) || !dt.reg(uart, 0, &base, &size)) {
        return false;
    }
    pl011::set_base(base);
    fdt::Prop irq;
    if (dt.get_prop(uart, "interrupts", &irq) && irq.len >= 12 && fdt::be32(irq.data) == 0) {
        g_uart_intid = 32 + fdt::be32(irq.data + 4);   // <0 n flags>: SPI n = INTID 32 + n
    }
    return true;
}

bool expect_abort(const char* what, void (*action)(), uint32_t want_ec, uint32_t want_fsc)
{
    uint64_t esr = 0, far = 0;
    kprintf("test: %s\n", what);
    trap::expect_fault();
    action();
    bool ok = trap::fault_seen(&esr, &far) && trap::ec(esr) == want_ec && (trap::iss(esr) & 0x3f) == want_fsc;
    kprintf("test: %s\n", ok ? "fault reported as expected" : "UNEXPECTED result");
    return ok;
}

} // namespace

extern "C" void kmain(const void* dtb, uint64_t entry_el)
{
    fdt::Blob dt;
    if (!dt.init(dtb) || !console(dt)) {
        arch::halt_forever();
    }
    psci::init(dt);
    kprintf("D2: entered at EL%lu, console at 0x%lx\n", entry_el, pl011::base());

    // Interrupt controller first: refuse what we do not support, clearly (acceptance test 2).
    gic::Version gv = gic::probe(dt);
    if (gv != gic::Version::V3) {
        kprintf("interrupt controller \"%s\": unsupported by this kernel (GICv3 only); stopping\n",
                gic::compatible());
        arch::qemu_exit(gv == gic::Version::V2 ? arch::kExitPass : arch::kExitFail);
    }

    // 1. MMU
    fdt::Node mem;
    uint64_t ram = 0, ram_size = 0;
    dt.find_path("/memory", &mem);
    dt.reg(mem, 0, &ram, &ram_size);
    mmu::init(ram, ram_size);
    mmu::explain(reinterpret_cast<uint64_t>(&kmain));
    mmu::explain(reinterpret_cast<uint64_t>(g_ret_insn));
    mmu::explain(pl011::base());
    mmu::explain(0x80000000);
    NeutralEnv env{arch::kName, dtb, arch::kPageSize};
    int failures = run_neutral_tests(env);   // again, now with the MMU and caches on

    bool f1 = expect_abort("write to the kernel's own code page", [] {
        // one 32-bit store (with -mstrict-align the compiler would split a plain C++ store
        // through a pointer of unknown alignment into byte stores, and we want exactly one fault)
        asm volatile("str wzr, [%0]" : : "r"(reinterpret_cast<uint64_t>(&kmain)) : "memory");
    }, 0x25, 0x0f);
    bool f2 = expect_abort("read 0x80000000 (no level-1 entry)", [] {
        (void)*reinterpret_cast<volatile uint32_t*>(0x80000000ull);
    }, 0x25, 0x05);
    bool f3 = expect_abort("call code placed in a data page", [] {
        reinterpret_cast<void (*)()>(reinterpret_cast<uint64_t>(g_ret_insn))();
    }, 0x21, 0x0f);

    // 2. GIC and timer
    gic::init_distributor();
    if (!gic::init_cpu()) {
        kprintf("no redistributor for this CPU\n");
        arch::qemu_exit(arch::kExitFail);
    }
    g_timer_intid = gtimer::intid_from_devicetree(dt);
    uint64_t hz = gtimer::freq();
    g_interval = hz / 100;                     // 10 ms
    kprintf("generic timer: CNTFRQ_EL0 %lu Hz; tick every %lu counts; timer INTID %u; UART INTID %u\n", hz,
            g_interval, g_timer_intid, g_uart_intid);
    trap::set_irq_handler(irq_handler);
    gic::enable_ppi(g_timer_intid, 0x80);
    gic::enable_spi(g_uart_intid, 0xa0, true);
    pl011::enable_rx_interrupt();
    uint64_t t0 = gtimer::now();
    gtimer::arm(g_interval);
    arch::sti();
    while (g_ticks < 50 && gtimer::now() - t0 < 5 * hz) {
        arch::wfi();
    }
    uint64_t elapsed = gtimer::now() - t0;
    arch::cli();
    gtimer::stop();
    uint64_t ticks = g_ticks.load();
    kprintf("ticks: %lu in %lu counts = %lu ms of counter time\n", ticks, elapsed, elapsed * 1000 / hz);
    gic::dump();
    bool ticks_ok = ticks == 50 && elapsed >= 50 * g_interval && elapsed < 60 * g_interval;

    uint32_t rx = g_rx.load(std::memory_order_acquire);
    g_rx_buf[rx] = '\0';
    for (uint32_t i = 0; i < rx; ++i) {
        if (g_rx_buf[i] == '\n' || g_rx_buf[i] == '\r') {
            g_rx_buf[i] = '/';
        }
    }
    kprintf("UART receive interrupts delivered %u characters: \"%s\"\n", rx, g_rx_buf);
    bool rx_ok = rx > 0;

    if (failures == 0 && f1 && f2 && f3 && ticks_ok && rx_ok) {
        kprintf("D2 ok\n");
        arch::qemu_exit(arch::kExitPass);
    }
    kprintf("D2 FAILED\n");
    arch::qemu_exit(arch::kExitFail);
}
