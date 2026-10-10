// pi_main.cc - F4-27: milestone D4 for a Raspberry Pi 3 (BCM2837), run here on QEMU's raspi3b.
// It differs from the virt kernel (F4-24 ... F4-26) in exactly the places a board port differs:
//   - the console: found through /aliases and translated through /soc's "ranges" (the Pi
//     devicetree writes VideoCore bus addresses), and the PL011 is programmed (baud rate,
//     pins) because a real board's firmware may leave it unconfigured;
//   - no devicetree at all is possible (QEMU without -dtb): then a board table chosen by the
//     CPU part number is the fallback, and the output says so;
//   - the other cores start through a spin table, not PSCI, and there is no PSCI to power
//     off: the end of the run is a watchdog reset (QEMU exits because of -no-reboot).
// The BCM2837 register addresses below are those of QEMU's raspi3b model, which this run
// exercises. Whether they match a real board, and every Pi 4 / Pi 5 value, is unverified.
#include <atomic>
#include <cstdint>
#include "arch.h"
#include "fdt.h"
#include "kprint.h"
#include "neutral_tests.h"
#include "pl011.h"
#include "spinlock.h"
#include "trap.h"

extern "C" char pi_secondary_entry[];

namespace {

// ---- the fallback board table: used only when there is no devicetree -------------------------
struct Board {
    uint32_t midr_part;          // MIDR_EL1 bits 15:4 of the board's cores
    const char* name;
    uint64_t periph_base;        // ARM physical address of the peripheral block
    uint64_t uart0;              // PL011
    uint64_t uart_clock;         // Hz, the PL011 reference clock the firmware sets up
    uint64_t release_addr[4];    // spin-table release addresses, one per core
};
// Pi 3 values: QEMU raspi3b's (this run checks them). Pi 4 values: recalled, unverified, and
// never used in this lab (QEMU 8.2 has no raspi4b machine). The Pi 5 is left out on purpose:
// its UART sits behind a different chip (chapter F4-27, unverified box).
constexpr Board kBoards[] = {
    {0xd03, "Raspberry Pi 3 (BCM2837, Cortex-A53)", 0x3f000000, 0x3f201000, 48000000, {0xd8, 0xe0, 0xe8, 0xf0}},
    {0xd08, "Raspberry Pi 4 (BCM2711, Cortex-A72) - UNVERIFIED VALUES", 0xfe000000, 0xfe201000, 48000000,
     {0xd8, 0xe0, 0xe8, 0xf0}},
};

struct Platform {
    const char* source = "none";
    uint64_t periph_base = 0;
    uint64_t uart = 0;
    uint64_t uart_clock = 0;
    uint64_t wdog = 0;              // power manager / watchdog block
    int ncores = 0;
    uint64_t release_addr[4] = {};
};
Platform g_pf;

// ---- devicetree helpers the virt kernels did not need ------------------------------------------

// /chosen/stdout-path may name an alias ("serial0:115200n8") instead of a path.
bool resolve_path(const fdt::Blob& dt, const char* in, uint32_t len, char* out, uint32_t cap)
{
    uint32_t i = 0;
    for (; i + 1 < cap && i < len && in[i] != '\0' && in[i] != ':'; ++i) {
        out[i] = in[i];
    }
    out[i] = '\0';
    if (out[0] == '/') {
        return true;
    }
    fdt::Node aliases;
    fdt::Prop p;
    if (!dt.find_path("/aliases", &aliases) || !dt.get_prop(aliases, out, &p)) {
        return false;
    }
    for (i = 0; i + 1 < cap && i < p.len && p.data[i] != '\0'; ++i) {
        out[i] = static_cast<char>(p.data[i]);
    }
    out[i] = '\0';
    return true;
}

// A node under /soc has a bus address; /soc's ranges = <child-base parent-base size> maps it to
// the CPU's physical address. One level of translation is all this devicetree needs.
uint64_t translate_soc(const fdt::Blob& dt, uint64_t bus)
{
    fdt::Node soc;
    fdt::Prop r;
    if (!dt.find_path("/soc", &soc) || !dt.get_prop(soc, "ranges", &r)) {
        return bus;                                  // no ranges: addresses are identical
    }
    for (uint32_t off = 0; off + 12 <= r.len; off += 12) {
        uint64_t child = fdt::be32(r.data + off), parent = fdt::be32(r.data + off + 4);
        uint64_t size = fdt::be32(r.data + off + 8);
        if (bus >= child && bus - child < size) {
            return parent + (bus - child);
        }
    }
    return bus;
}

bool platform_from_devicetree(const fdt::Blob& dt)
{
    fdt::Node chosen, uart, n;
    fdt::Prop p;
    char path[64];
    if (!dt.find_path("/chosen", &chosen) || !dt.get_prop(chosen, "stdout-path", &p) ||
        !resolve_path(dt, reinterpret_cast<const char*>(p.data), p.len, path, sizeof path) ||
        !dt.find_path(path, &uart) || !dt.is_compatible(uart, "arm,pl011")) {
        return false;
    }
    uint64_t bus = 0, size = 0;
    dt.reg(uart, 0, &bus, &size);
#ifdef FORENSIC_NO_RANGES
    g_pf.uart = bus;                                 // the bug: a bus address used as physical
#else
    g_pf.uart = translate_soc(dt, bus);
#endif
    g_pf.uart_clock = dt.get_prop(uart, "clock-frequency", &p) ? fdt::be32(p.data) : 48000000;
    if (dt.find_compatible("brcm,bcm2835-pm-wdt", &n) && dt.reg(n, 0, &bus, &size)) {
        g_pf.wdog = translate_soc(dt, bus);
    }
    g_pf.ncores = 0;
    dt.for_each_node([&](const fdt::Node& c) {
        fdt::Prop q;
        if (g_pf.ncores < 4 && dt.get_prop(c, "device_type", &q) &&
            fdt::streq(reinterpret_cast<const char*>(q.data), "cpu")) {
            uint64_t rel = 0;
            if (dt.get_prop(c, "enable-method", &q) &&
                fdt::streq(reinterpret_cast<const char*>(q.data), "spin-table") &&
                dt.get_prop(c, "cpu-release-addr", &q) && q.len == 8) {
                rel = uint64_t{fdt::be32(q.data)} << 32 | fdt::be32(q.data + 4);
            }
            g_pf.release_addr[g_pf.ncores++] = rel;
        }
        return true;
    });
    g_pf.source = "devicetree";
    return true;
}

bool platform_from_board_table()
{
    uint32_t part = static_cast<uint32_t>(READ_SYSREG(midr_el1) >> 4) & 0xfff;
    for (const Board& b : kBoards) {
        if (b.midr_part == part) {
            g_pf.periph_base = b.periph_base;
            g_pf.uart = b.uart0;
            g_pf.uart_clock = b.uart_clock;
            g_pf.wdog = b.periph_base + 0x100000;
            g_pf.ncores = 4;
            for (int i = 0; i < 4; ++i) {
                g_pf.release_addr[i] = b.release_addr[i];
            }
            g_pf.source = b.name;
            return true;
        }
    }
    return false;
}

// ---- the PL011 set-up a real board needs (QEMU accepts it and ignores the timing) -----------
// Offsets and bits after the PL011 TRM (title only, pending verification).
constexpr uint32_t kFR = 0x18 / 4, kIBRD = 0x24 / 4, kFBRD = 0x28 / 4, kLCRH = 0x2c / 4, kCR = 0x30 / 4;
constexpr uint32_t kFR_BUSY = 1u << 3;

struct Divisor {
    uint32_t ibrd, fbrd;
};
// baud divisor = clock / (16 * baud), as a 16.6 fixed-point number, rounded to nearest
Divisor pl011_divisor(uint64_t clock, uint32_t baud)
{
    uint64_t div64 = (clock * 4 + baud / 2) / baud;   // = 64 * clock / (16 * baud)
    return {static_cast<uint32_t>(div64 >> 6), static_cast<uint32_t>(div64 & 63)};
}

void pl011_configure(uint64_t base, uint64_t clock, uint32_t baud)
{
    volatile uint32_t* u = reinterpret_cast<volatile uint32_t*>(base);
    u[kCR] = 0;                                        // disable while changing the format
    while (u[kFR] & kFR_BUSY) {
    }
    Divisor d = pl011_divisor(clock, baud);
    u[kIBRD] = d.ibrd;
    u[kFBRD] = d.fbrd;
    u[kLCRH] = (3u << 5) | (1u << 4);                  // 8 data bits, FIFOs on (LCRH written last)
    u[kCR] = (1u << 0) | (1u << 8) | (1u << 9);        // UART enable, TX enable, RX enable
}

// GPIO 14 and 15 to function ALT0 (the PL011's TXD0/RXD0 on a BCM2837). GPFSEL1 holds pins
// 10-19, three bits each; 0b100 = ALT0. Unverified register details; QEMU models the block.
void uart_pins(uint64_t periph_base)
{
    volatile uint32_t* gpfsel1 = reinterpret_cast<volatile uint32_t*>(periph_base + 0x200000 + 0x04);
    uint32_t v = *gpfsel1;
    v &= ~((7u << 12) | (7u << 15));
    v |= (4u << 12) | (4u << 15);
    *gpfsel1 = v;
}

// ---- secondary cores: spin table ----------------------------------------------------------------
struct alignas(64) Core {
    uint64_t index;
    uint64_t entry_el;
};
Core g_core[4];
std::atomic<int> g_online{0};
std::atomic<int> g_go{0};
TicketLock g_lock;
uint64_t g_locked = 0;
constexpr uint32_t kRounds = 20000;

void lock_rounds()
{
    for (uint32_t i = 0; i < kRounds; ++i) {
        g_lock.lock();
        ++g_locked;
        g_lock.unlock();
    }
}

void release_core(uint64_t addr, uint64_t entry)
{
    volatile uint64_t* slot = reinterpret_cast<volatile uint64_t*>(addr);
    *slot = entry;
    // The waiting core reads the slot with its caches off: push the line to memory, then wake it.
    asm volatile("dc civac, %0\n\tdsb sy\n\tsev" : : "r"(addr) : "memory");
}

uint64_t now() { return READ_SYSREG(cntpct_el0); }

} // namespace

extern "C" [[noreturn]] void secondary_main(uint64_t index, uint64_t entry_el)
{
    g_core[index].index = index;
    g_core[index].entry_el = entry_el;
    WRITE_SYSREG(tpidr_el1, reinterpret_cast<uint64_t>(&g_core[index]));
    g_lock.lock();
    kprintf("core %lu up: entered at EL%lu, running at EL%lu, MPIDR 0x%lx\n", index, entry_el,
            arch::current_el(), READ_SYSREG(mpidr_el1));
    g_lock.unlock();
    g_online.fetch_add(1, std::memory_order_release);
    while (g_go.load(std::memory_order_acquire) == 0) {
        arch::spin_wait();
    }
    lock_rounds();
    g_online.fetch_add(1, std::memory_order_release);
    arch::halt_forever();
}

namespace arch {

// No PSCI on this board: end the run with the power-management watchdog. Password 0x5a in the
// top byte; RSTC full-reset value 0x20; WDOG = time-out in watchdog ticks (unverified values;
// QEMU's model resets, and -no-reboot turns the reset into a QEMU exit).
[[noreturn]] void qemu_exit(uint8_t code)
{
    kprintf("exit: %s (code 0x%x); watchdog reset at 0x%lx\n", code == kExitPass ? "pass" : "FAIL", code, g_pf.wdog);
    if (g_pf.wdog != 0) {
        volatile uint32_t* pm = reinterpret_cast<volatile uint32_t*>(g_pf.wdog);
        pm[0x24 / 4] = 0x5a000000 | 10;     // PM_WDOG: 10 ticks
        pm[0x1c / 4] = 0x5a000000 | 0x20;   // PM_RSTC: full reset
    }
    halt_forever();
}

} // namespace arch

extern "C" void kmain(const void* dtb, uint64_t entry_el)
{
    fdt::Blob dt;
    bool have_dt = dt.init(dtb);
    bool found = have_dt ? platform_from_devicetree(dt) : platform_from_board_table();
    if (!found) {
        arch::halt_forever();                      // no console known: nothing can be reported
    }
    if (g_pf.periph_base == 0) {
        g_pf.periph_base = g_pf.uart & ~uint64_t{0xffffff};   // the 16 MiB peripheral window
    }
    uart_pins(g_pf.periph_base);
    pl011_configure(g_pf.uart, g_pf.uart_clock, 115200);
    pl011::set_base(g_pf.uart);

    Divisor d = pl011_divisor(g_pf.uart_clock, 115200);
    kprintf("D4: platform from %s\n", g_pf.source);
    if (have_dt) {
        kprintf("devicetree at %p, %u bytes; console %s -> 0x%lx after /soc ranges\n", dtb, dt.total_size(),
                "stdout-path", g_pf.uart);
    } else {
        kprintf("no devicetree in x0 (x0 = %p): board table chosen by MIDR part number\n", dtb);
    }
    kprintf("PL011 at 0x%lx, clock %lu Hz, 115200 baud: IBRD %u FBRD %u\n", g_pf.uart, g_pf.uart_clock, d.ibrd, d.fbrd);
    kprintf("entered at EL%lu, running at EL%lu\n", entry_el, arch::current_el());
    uint64_t midr = READ_SYSREG(midr_el1);
    kprintf("MIDR_EL1 0x%lx (part 0x%lx)\n", midr, midr >> 4 & 0xfff);
    uint64_t freq = READ_SYSREG(cntfrq_el0);
    kprintf("CNTFRQ_EL0 %lu Hz\n", freq);

    NeutralEnv env{arch::kName, have_dt ? dtb : nullptr, arch::kPageSize};
    int failures = run_neutral_tests(env);

    // the other cores, through the spin table
    int started = 0;
    for (int i = 1; i < g_pf.ncores; ++i) {
        if (g_pf.release_addr[i] == 0) {
            kprintf("core %d: no spin-table release address\n", i);
            continue;
        }
        g_lock.lock();
        kprintf("core %d: writing entry 0x%lx to release address 0x%lx\n", i,
                reinterpret_cast<uint64_t>(pi_secondary_entry), g_pf.release_addr[i]);
        g_lock.unlock();
        release_core(g_pf.release_addr[i], reinterpret_cast<uint64_t>(pi_secondary_entry));
        ++started;
    }
    for (uint64_t t0 = now(); g_online.load(std::memory_order_acquire) < started && now() - t0 < freq;) {
        arch::cpu_relax();
    }
    int up = g_online.load();
    kprintf("%d of %d other cores came up\n", up, started);
    g_online.store(0);
    g_go.store(1, std::memory_order_release);
    arch::spin_wake();
    lock_rounds();
    for (uint64_t t0 = now(); g_online.load(std::memory_order_acquire) < up && now() - t0 < 10 * freq;) {
        arch::cpu_relax();
    }
    uint64_t want = uint64_t{kRounds} * static_cast<uint64_t>(up + 1);
    kprintf("lock test: %d cores x %u rounds: counter %lu (expected %lu)\n", up + 1, kRounds, g_locked, want);

    // 100 ms of the generic timer, polled (the BCM2837 has no GIC; its interrupt wiring is
    // board-specific and is left to the project)
    uint64_t t0 = now();
    while (now() - t0 < freq / 10) {
    }
    kprintf("generic timer: %lu ticks = 100 ms at CNTFRQ\n", now() - t0);

    if (failures == 0 && up == started && g_locked == want) {
        kprintf("D4 ok\n");
        arch::qemu_exit(arch::kExitPass);
    }
    kprintf("D4 FAILED\n");
    arch::qemu_exit(arch::kExitFail);
}
