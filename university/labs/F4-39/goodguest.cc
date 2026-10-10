// goodguest.cc - F4-39: a kernel that behaves well as a guest (milestone V1, partly).
// Command line (Multiboot): test=detect | test=idle idle=hlt|poll seconds=N
//                           | test=mono reads=N | test=touch addr=N (decimal)
#include "hvdetect.h"
#include "intr.h"
#include "kio.h"

namespace {
uint64_t g_tsc_hz;   // TSC ticks per second, measured against the PIT

void print_signature(const char* sig)
{
    for (int i = 0; i < 12; ++i) {
        char c = sig[i];
        kprintf("%c", (c >= 32 && c < 127) ? c : '.');
    }
}

void report(const HvInfo& hv)
{
    kprintf("hypervisor-present bit (CPUID.1:ECX[31]): %u\n", hv.present ? 1u : 0u);
    if (hv.present) {
        kprintf("CPUID 0x40000000: max leaf %x, signature \"", hv.max_leaf);
        print_signature(hv.signature);
        kprintf("\"\n");
    }
    kprintf("hypervisor: %s\n", hv.name);
    if (hv.kvm_features != 0) {
        kprintf("KVM features (CPUID 0x40000001 EAX): %x, kvmclock (bit 3): %s\n",
                hv.kvm_features, (hv.kvm_features >> 3) & 1 ? "yes" : "no");
    }
}

// Calibrate the TSC against PIT ticks while halting (interrupts must be running).
void calibrate_tsc(unsigned hz)
{
    uint64_t t = ticks();
    while (ticks() == t) { asm volatile("hlt"); }       // align to a tick edge
    uint64_t start = rdtsc();
    uint64_t first = ticks();
    while (ticks() < first + hz / 5) { asm volatile("hlt"); }   // 0.2 s
    g_tsc_hz = (rdtsc() - start) * 5;
}

// Idle the right way: halt until the next interrupt. The vCPU really stops.
uint64_t wait_hlt(uint64_t n_ticks)
{
    uint64_t loops = 0;
    uint64_t end = ticks() + n_ticks;
    while (ticks() < end) {
        asm volatile("hlt");
        ++loops;
    }
    return loops;
}

// Idle the wrong way: interrupts off, spin reading the PIT counter, count its wraps.
uint64_t wait_poll(uint64_t n_ticks)
{
    irq_disable();
    uint64_t loops = 0;
    uint64_t wraps = 0;
    uint16_t last = pit_read_counter();
    while (wraps < n_ticks) {
        uint16_t now = pit_read_counter();
        if (now > last) {          // channel 0 counts down; a jump up is a reload
            ++wraps;
        }
        last = now;
        ++loops;
    }
    irq_enable();
    return loops;
}

int test_idle(const HvInfo& hv)
{
    const unsigned hz = 100;
    uint64_t seconds = arg_num("seconds", 3);
    bool poll = arg_is("idle", "poll");
    kprintf("idle test: %s for %lu s at %u Hz (%s)\n", poll ? "busy-poll the PIT" : "hlt",
            seconds, hz, hv.name);
    uint64_t t0 = rdtsc();
    uint64_t loops = poll ? wait_poll(seconds * hz) : wait_hlt(seconds * hz);
    uint64_t t1 = rdtsc();
    kprintf("waited %lu ticks; loop iterations %lu; TSC delta %lu\n", seconds * hz, loops,
            t1 - t0);
    return 0;
}

int test_mono(uint64_t reads)
{
    // Clock source: the TSC converted to nanoseconds with the calibrated rate.
    // (Under KVM a good guest would use kvmclock here: see pvclock_model.cpp.)
    uint64_t last = 0;
    uint64_t backwards = 0;
    uint64_t t0 = rdtsc();
    for (uint64_t i = 0; i < reads; ++i) {
        uint64_t d = rdtsc() - t0;
        uint64_t ns = (d / g_tsc_hz) * 1000000000ull + (d % g_tsc_hz) * 1000000000ull / g_tsc_hz;
        if (ns < last) {
            ++backwards;
        }
        last = ns;
    }
    kprintf("monotonic test: %lu reads, %lu went backwards, last %lu ns\n", reads, backwards,
            last);
    return backwards == 0 ? 0 : 1;
}
}

extern "C" [[noreturn]] void kmain(uint32_t magic, uint32_t info)
{
    serial_init();
    cmdline_init(magic, info);
    kprintf("DR404 good-guest kernel (F4-39); cmdline \"%s\"\n", cmdline());
    idt_init();
    HvInfo hv = detect_hypervisor();
    report(hv);
    timer_start(100);
    irq_enable();
    calibrate_tsc(100);
    kprintf("clock source: tsc, calibrated against the PIT: %lu Hz (measured)\n", g_tsc_hz);
    int rc = 0;
    if (arg_is("test", "idle")) {
        rc = test_idle(hv);
    } else if (arg_is("test", "mono")) {
        rc = test_mono(arg_num("reads", 10000000));
    } else if (arg_is("test", "touch")) {      // read one byte at a physical address
        uint64_t addr = arg_num("addr", 0x40000000);
        uint8_t b = *reinterpret_cast<volatile uint8_t*>(addr);
        kprintf("read %lx: %x\n", addr, b);
    }
    kprintf("done: %s\n", rc == 0 ? "PASS" : "FAIL");
    qemu_exit(rc == 0 ? 0 : 1);
}
