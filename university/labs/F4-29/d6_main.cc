// d6_main.cc - F4-29: milestone D6's arch pieces (and a D7 stand-in on QEMU's model of a real
// board): SBI timer ticks, PLIC external interrupts from the UART, harts started through SBI
// HSM, per-hart data through the tp register, a lock tested by every hart, and IPIs.
#include <atomic>
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "kprint.h"
#include "plic.h"
#include "sbi.h"
#include "spinlock.h"
#include "sv39.h"
#include "trap.h"
#include "uart_any.h"

extern "C" char secondary_entry[];

namespace {

constexpr int kMaxHarts = 8;
struct Hart {
    uint64_t hartid;
    int index;
    std::atomic<uint32_t> ipis{0};
};
Hart g_hart[kMaxHarts];
int g_nharts = 0;                       // harts this kernel will use
std::atomic<int> g_online{0}, g_start{0}, g_done{0};
TicketLock g_lock, g_print;
uint64_t g_locked = 0;
std::atomic<uint64_t> g_racy{0};
uint32_t g_rounds = 50000;
alignas(4096) uint64_t g_root[512];

std::atomic<uint64_t> g_ticks{0};
uint64_t g_tick_interval = 0;
uint32_t g_uart_source = 0;
std::atomic<uint32_t> g_rx{0};
char g_rx_buf[32];

Hart& me() { uint64_t tp; asm volatile("mv %0, tp" : "=r"(tp)); return *reinterpret_cast<Hart*>(tp); }
uint64_t now() { return CSR_READ(time); }

constexpr uint64_t kSSIE = 1u << 1, kSTIE = 1u << 5, kSEIE = 1u << 9;

void irq(TrapFrame*, uint64_t code)
{
    if (code == 5) {                                   // supervisor timer
        g_ticks.fetch_add(1, std::memory_order_relaxed);
        sbi::set_timer(now() + g_tick_interval);       // next deadline (also clears the pending bit)
    } else if (code == 1) {                            // supervisor software interrupt: an IPI
        CSR_CLEAR(sip, kSSIE);
        me().ipis.fetch_add(1, std::memory_order_relaxed);
    } else if (code == 9) {                            // supervisor external: ask the PLIC who
        uint32_t src = plic::claim(me().hartid);
        if (src == g_uart_source) {
            char c;
            while (uart::read_char(&c)) {
                uint32_t n = g_rx.load(std::memory_order_relaxed);
                if (n + 1 < sizeof g_rx_buf) {
                    g_rx_buf[n] = c == '\n' ? '/' : c;
                    g_rx.store(n + 1, std::memory_order_release);
                }
            }
        }
#ifndef FORENSIC_NO_COMPLETE
        if (src != 0) {
            plic::complete(me().hartid, src);          // the source may interrupt again
        }
#endif
    }
}

void hart_setup(Hart& h)
{
    asm volatile("mv tp, %0" : : "r"(&h));               // per-hart data pointer
    CSR_WRITE(satp, sv39::satp_value(g_root));
    arch::sfence_vma();
    plic::init_hart(h.hartid);
    trap::set_irq_handler(irq);
    CSR_SET(sie, kSSIE | kSEIE);
    arch::sti();
}

void lock_test()
{
    while (g_start.load(std::memory_order_acquire) == 0) {
        arch::cpu_relax();
    }
    for (uint32_t i = 0; i < g_rounds; ++i) {
        g_lock.lock();
        ++g_locked;
        g_lock.unlock();
        g_racy.store(g_racy.load(std::memory_order_relaxed) + 1, std::memory_order_relaxed);
    }
    g_done.fetch_add(1, std::memory_order_acq_rel);
}

bool console(const fdt::Blob& dt)
{
    fdt::Node chosen, n;
    fdt::Prop p;
    if (!dt.find_path("/chosen", &chosen) || !dt.get_prop(chosen, "stdout-path", &p)) {
        return false;
    }
    char path[64];
    uint32_t i = 0;
    for (; i + 1 < sizeof path && i < p.len && p.data[i] != '\0' && p.data[i] != ':'; ++i) {
        path[i] = static_cast<char>(p.data[i]);
    }
    path[i] = '\0';
    if (!dt.find_path(path, &n) || !uart::init_from(dt, n)) {
        return false;
    }
    fdt::Prop irqp;
    if (dt.get_prop(n, "interrupts", &irqp) && irqp.len >= 4) {
        g_uart_source = fdt::be32(irqp.data);
    }
    kprintf("console: %s (%s) at 0x%lx, PLIC source %u\n", path, uart::kind(), uart::base(), g_uart_source);
    return true;
}

// Harts from the devicetree. A hart is used only if its node says it can run this kernel:
// status "okay" (or absent) and an mmu-type (S-mode with paging).
void find_harts(const fdt::Blob& dt, uint64_t boot_hart)
{
    dt.for_each_node([&](const fdt::Node& n) {
        fdt::Prop p;
        uint64_t hart = 0, unused = 0;
        if (!dt.get_prop(n, "device_type", &p) || !fdt::streq(reinterpret_cast<const char*>(p.data), "cpu") ||
            !dt.reg(n, 0, &hart, &unused)) {
            return true;
        }
        fdt::Prop st, mmu, isa;
        bool okay = !dt.get_prop(n, "status", &st) || fdt::streq(reinterpret_cast<const char*>(st.data), "okay");
        bool has_mmu = dt.get_prop(n, "mmu-type", &mmu);
        dt.get_prop(n, "riscv,isa", &isa);
        bool use = okay && has_mmu;
#ifdef FORENSIC_ALL_HARTS
        use = true;                                    // the forensic kernel: "every hart is like hart 0"
#endif
        kprintf("hart %lu: isa %s, mmu-type %s, status %s -> %s\n", hart, reinterpret_cast<const char*>(isa.data),
                has_mmu ? reinterpret_cast<const char*>(mmu.data) : "(none)",
                dt.get_prop(n, "status", &st) ? reinterpret_cast<const char*>(st.data) : "(none)",
                use ? (hart == boot_hart ? "use (boot hart)" : "use") : "skip");
        if (use && g_nharts < kMaxHarts) {
            g_hart[g_nharts].hartid = hart;
            g_hart[g_nharts].index = g_nharts;
            ++g_nharts;
        }
        return true;
    });
}

void parse_bootargs(const fdt::Blob& dt)
{
    fdt::Node chosen;
    fdt::Prop p;
    if (!dt.find_path("/chosen", &chosen) || !dt.get_prop(chosen, "bootargs", &p)) {
        return;
    }
    for (const char* s = reinterpret_cast<const char*>(p.data); *s != '\0'; ++s) {
        if (s[0] == 'r' && s[1] == 'o' && s[2] == 'u' && s[3] == 'n' && s[4] == 'd' && s[5] == 's' && s[6] == '=') {
            g_rounds = 0;
            for (s += 7; *s >= '0' && *s <= '9'; ++s) {
                g_rounds = g_rounds * 10 + static_cast<uint32_t>(*s - '0');
            }
            return;
        }
    }
}

} // namespace

extern "C" [[noreturn]] void secondary_main(uint64_t hartid, uint64_t index)
{
    hart_setup(g_hart[index]);
    g_print.lock();
    kprintf("hart %lu online (index %lu), tp -> g_hart[%d]\n", hartid, index, me().index);
    g_print.unlock();
    g_online.fetch_add(1, std::memory_order_release);
    lock_test();
    for (;;) {
        arch::wfi();
    }
}

extern "C" void kmain(uint64_t boot_hart, const void* dtb)
{
    fdt::Blob dt;
    if (!dt.init(dtb) || !console(dt)) {
        arch::halt_forever();
    }
    parse_bootargs(dt);
    fdt::Node cpus, mem;
    fdt::Prop tb;
    uint64_t timebase = dt.find_path("/cpus", &cpus) && dt.get_prop(cpus, "timebase-frequency", &tb) ? fdt::be32(tb.data) : 0;
    kprintf("D6: boot hart %lu; timebase-frequency %lu Hz (from the devicetree)\n", boot_hart, timebase);

    uint64_t ram = 0, ram_size = 0;
    dt.find_path("/memory", &mem);
    dt.reg(mem, 0, &ram, &ram_size);
    g_root[0] = sv39::leaf(0, sv39::kR | sv39::kW);                       // devices
    g_root[ram / sv39::kGiB] = sv39::leaf(ram & ~(sv39::kGiB - 1), sv39::kR | sv39::kW | sv39::kX);
    if (!plic::probe(dt)) {
        kprintf("no PLIC\n");
        arch::qemu_exit(arch::kExitFail);
    }
    find_harts(dt, boot_hart);
    int boot_index = 0;
    for (int i = 0; i < g_nharts; ++i) {
        if (g_hart[i].hartid == boot_hart) {
            boot_index = i;
        }
    }
    hart_setup(g_hart[boot_index]);
    g_online.store(1);

    // Timer: SBI TIME extension; 10 ms ticks counted against the time CSR
    g_tick_interval = timebase / 100;
    uint64_t t0 = now();
    sbi::set_timer(t0 + g_tick_interval);
    CSR_SET(sie, kSTIE);
    while (g_ticks.load() < 20 && now() - t0 < 5 * timebase) {
        arch::wfi();
    }
    uint64_t elapsed = now() - t0;
    uint64_t ticks = g_ticks.load();   // the timer keeps running: it also ends the waits below
    kprintf("timer: %lu ticks in %lu time units = %lu ms\n", ticks, elapsed, elapsed * 1000 / timebase);
    bool timer_ok = ticks == 20 && elapsed >= 20 * g_tick_interval && elapsed < 25 * g_tick_interval;

    // UART receive through the PLIC (only where the console UART has a receive interrupt here)
    bool rx_tested = false;
    if (fdt::streq(uart::kind(), "ns16550a") && g_uart_source != 0) {
        rx_tested = true;
        plic::enable(boot_hart, g_uart_source, 1);
        uart::enable_rx_interrupt();
        uint64_t w0 = now();
        while (g_rx.load() < 6 && now() - w0 < 10 * timebase) {
            arch::wfi();
        }
        uint32_t n = g_rx.load(std::memory_order_acquire);
        g_rx_buf[n] = '\0';
        kprintf("UART: %u characters through PLIC source %u: \"%s\"\n", n, g_uart_source, g_rx_buf);
        plic::dump(boot_hart, g_uart_source);
    }
    bool rx_ok = !rx_tested || g_rx.load() == 6;

    // Other harts through SBI HSM
    for (int i = 0; i < g_nharts; ++i) {
        if (i == boot_index) {
            continue;
        }
        sbi::Ret r = sbi::hart_start(g_hart[i].hartid, reinterpret_cast<uint64_t>(secondary_entry),
                                     static_cast<uint64_t>(i));
        kprintf("SBI hart_start(hart %lu) = error %ld\n", g_hart[i].hartid, r.error);
    }
    for (uint64_t w0 = now(); g_online.load(std::memory_order_acquire) < g_nharts && now() - w0 < 2 * timebase;) {
        arch::cpu_relax();
    }
    for (int i = 0; i < g_nharts; ++i) {
        kprintf("SBI hart_status(hart %lu) = %ld\n", g_hart[i].hartid, sbi::hart_status(g_hart[i].hartid).value);
    }
    if (g_online.load() < g_nharts) {
        kprintf("only %d of %d harts came online\n", g_online.load(), g_nharts);
        kprintf("D6 FAILED\n");
        arch::qemu_exit(arch::kExitFail);
    }

    g_start.store(1, std::memory_order_release);
    lock_test();
    while (g_done.load(std::memory_order_acquire) < g_nharts) {
        arch::cpu_relax();
    }
    uint64_t want = uint64_t{g_rounds} * static_cast<uint64_t>(g_nharts);
    kprintf("lock test: %d harts x %u rounds: locked counter %lu (expected %lu), racy counter %lu\n", g_nharts,
            g_rounds, g_locked, want, g_racy.load());

    uint64_t mask = 0;
    for (int i = 0; i < g_nharts; ++i) {
        if (i != boot_index) {
            mask |= uint64_t{1} << g_hart[i].hartid;
        }
    }
    sbi::send_ipi(mask, 0);
    bool ipi_ok = true;
    for (int i = 0; i < g_nharts; ++i) {
        if (i == boot_index) {
            continue;
        }
        for (uint64_t w0 = now(); g_hart[i].ipis.load() == 0 && now() - w0 < timebase;) {
            arch::cpu_relax();
        }
        ipi_ok = ipi_ok && g_hart[i].ipis.load() == 1;
    }
    kprintf("IPI (SBI send_ipi, hart mask 0x%lx): every other hart took exactly one: %s\n", mask, ipi_ok ? "yes" : "NO");

    CSR_CLEAR(sie, kSTIE);
    if (timer_ok && rx_ok && ipi_ok && g_locked == want) {
        kprintf("D6 ok\n");
        arch::qemu_exit(arch::kExitPass);
    }
    kprintf("D6 FAILED\n");
    arch::qemu_exit(arch::kExitFail);
}
