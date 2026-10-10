// hv.cc - the DR404 tiny hypervisor: SVM enablement, VMCB set-up, the run loop and the
// exit handlers (I/O with a 16550 output model and an 8254 model, CPUID, HLT, VMMCALL,
// triple fault, failed entry). MSR numbers and CPUID bits marked "APM" are from memory
// of the AMD64 APM Vol. 2 (not opened in this build); the runs prove them on QEMU only.
#include "hv.h"
#include "intr.h"
#include "kio.h"

uint64_t g_tsc_hz;

namespace {
constexpr uint32_t MSR_EFER = 0xC0000080;
constexpr uint64_t EFER_SVME = 1ull << 12;         // APM: EFER bit 12 enables SVM
constexpr uint32_t MSR_VM_CR = 0xC0010114;         // APM: bit 4 SVMDIS (BIOS lock-out)
constexpr uint32_t MSR_VM_HSAVE_PA = 0xC0010117;   // APM: where VMRUN saves host state
constexpr uint32_t PIT_HZ = 1193182;               // 8254 input clock (PC design)

alignas(4096) uint8_t g_hsave[4096];
alignas(4096) uint8_t g_iopm[3 * 4096];            // I/O permission map: 1 bit per port

struct Name { uint64_t code; const char* name; };
constexpr Name kNames[] = {
    {SVM_EXIT_INTR, "INTR (host interrupt)"}, {SVM_EXIT_CPUID, "CPUID"},
    {SVM_EXIT_HLT, "HLT"}, {SVM_EXIT_IOIO, "IOIO (port I/O)"}, {SVM_EXIT_MSR, "MSR"},
    {SVM_EXIT_SHUTDOWN, "SHUTDOWN (triple fault)"}, {SVM_EXIT_VMRUN, "VMRUN"},
    {SVM_EXIT_VMMCALL, "VMMCALL (hypercall)"}, {SVM_EXIT_NPF, "NPF (nested page fault)"},
    {SVM_EXIT_VINTR, "VINTR"}, {static_cast<uint64_t>(SVM_EXIT_ERR), "INVALID (VMRUN failed)"},
};

void count_exit(Vcpu& v, uint64_t code)
{
    ++v.exits;
    for (ExitCount& e : v.by_reason) {
        if (e.count != 0 && e.code == code) { ++e.count; return; }
        if (e.count == 0) { e.code = code; e.count = 1; return; }
    }
}

void count_port(Vcpu& v, uint16_t port)
{
    for (PortCount& p : v.by_port) {
        if (p.count != 0 && p.port == port) { ++p.count; return; }
        if (p.count == 0) { p.port = port; p.count = 1; return; }
    }
}

void flush_line(Vcpu& v)                     // print one guest line on the host console
{
    if (v.line_len == 0) { return; }
    v.line[v.line_len] = 0;
    kprintf("  guest| %s\n", v.line);
    v.line_len = 0;
}

uint64_t pit_elapsed(const VirtualPit& p)   // PIT input-clock periods since programming
{
    uint64_t d = rdtsc() - p.start_tsc;
    return (d / g_tsc_hz) * PIT_HZ + (d % g_tsc_hz) * PIT_HZ / g_tsc_hz;
}

uint16_t pit_count(const VirtualPit& p)     // mode 2: counts reload .. 1, then reloads
{
    if (p.reload == 0) {
        return 0;
    }
    return static_cast<uint16_t>(p.reload - pit_elapsed(p) % p.reload);
}

// The emulated devices. Returns false if the access is not ours (then the hook decides).
bool emulate_io(Vcpu& v, uint16_t port, bool in, uint32_t& val)
{
    VirtualPit& p = v.pit;
    if (port == 0x3F8 && !in) {                      // 16550 transmit register
        char c = static_cast<char>(val);
        if (c != '\n') { v.line[v.line_len++] = c; }
        if (c == '\n' || v.line_len == sizeof v.line - 1) { flush_line(v); }
        return true;
    }
    if (port == 0x3FD && in) { val = 0x60; return true; }   // line status: transmitter empty
    if (port >= 0x3F8 && port <= 0x3FF) { val = 0; return true; }   // other UART registers
    if (port == 0x43 && !in) {                       // PIT mode/command register
        if ((val & 0xC0) == 0 && (val & 0x30) == 0) {    // channel 0 counter latch
            p.latched = pit_count(p); p.have_latch = true; p.read_high = false;
        } else if ((val & 0xC0) == 0) {                   // channel 0 new mode: lo/hi follow
            p.write_high = false;
        }
        return true;
    }
    if (port == 0x40) {
        if (!in) {                                    // reload value, low byte then high
            if (!p.write_high) { p.reload = static_cast<uint16_t>((p.reload & 0xFF00) | (val & 0xFF)); }
            else { p.reload = static_cast<uint16_t>((p.reload & 0x00FF) | ((val & 0xFF) << 8));
                   p.start_tsc = rdtsc(); }
            p.write_high = !p.write_high;
            return true;
        }
        uint16_t c = p.have_latch ? p.latched : pit_count(p);
        val = p.read_high ? (c >> 8) : (c & 0xFF);
        if (p.read_high) { p.have_latch = false; }
        p.read_high = !p.read_high;
        return true;
    }
    if (port == 0xF4 && !in) {                        // isa-debug-exit: the guest is done
        v.guest_exit_code = val; v.stop = Stop::GuestExit;
        return true;
    }
    return false;
}

void handle_ioio(Vcpu& v)
{
    uint64_t info = v.vmcb->u64(vmcb::EXITINFO1);
    uint16_t port = static_cast<uint16_t>(info >> 16);     // APM: bits 31-16 port
    bool in = info & 1;                                     // APM: bit 0 type, 1 = IN
    unsigned size = (info & 0x10) ? 1 : (info & 0x20) ? 2 : 4;   // APM: SZ8/SZ16/SZ32
    count_port(v, port);
    if (info & 0x4) {                                       // APM: bit 2 string (INS/OUTS)
        kprintf("hv: string I/O on port %x not supported\n", port);
        v.stop = Stop::Unhandled;
        return;
    }
    uint64_t& rax = v.vmcb->u64(vmcb::RAX);
    uint32_t val = in ? 0xFFFFFFFFu : static_cast<uint32_t>(rax);
    if (!emulate_io(v, port, in, val) && !(v.io_hook && v.io_hook(v, port, in, val))) {
        val = 0xFFFFFFFFu;                                  // nobody there: reads float high
    }
    if (in) {
        uint64_t mask = size == 1 ? 0xFF : size == 2 ? 0xFFFF : 0xFFFFFFFF;
        rax = (size == 4) ? (val & mask) : ((rax & ~mask) | (val & mask));
    }
    v.vmcb->u64(vmcb::RIP) = v.vmcb->u64(vmcb::EXITINFO2);    // APM: next RIP for IOIO
}

void handle_cpuid(Vcpu& v)
{
    uint64_t& rax = v.vmcb->u64(vmcb::RAX);
    uint32_t leaf = static_cast<uint32_t>(rax);
    CpuidRegs r = cpuid(leaf, static_cast<uint32_t>(v.regs.rcx));
    if (leaf == 1) {
        r.ecx |= 1u << 31;                     // hypervisor-present bit
    } else if (leaf == 0x80000001) {
        r.ecx &= ~(1u << 2);                   // APM: hide SVM itself (no nesting here)
    } else if (leaf == 0x40000000) {
        r.eax = 0x40000000;                    // our highest hypervisor leaf
        memcpy(&r.ebx, "DR40", 4);             // signature "DR404-tinyHV"
        memcpy(&r.ecx, "4-ti", 4);
        memcpy(&r.edx, "nyHV", 4);
    } else if (leaf > 0x40000000 && leaf < 0x40000100) {
        r = CpuidRegs{0, 0, 0, 0};
    }
    rax = r.eax;
    v.regs.rbx = r.ebx;
    v.regs.rcx = r.ecx;
    v.regs.rdx = r.edx;
    v.vmcb->u64(vmcb::RIP) += 2;               // CPUID is 0F A2
}

void handle_hlt(Vcpu& v)
{
    v.vmcb->u64(vmcb::RIP) += 1;               // HLT is F4
    if (v.halt_hook && v.halt_hook(v)) {
        return;                                // F4-41: an interrupt will wake the guest
    }
    if (!(v.vmcb->u64(vmcb::RFLAGS) & 0x200) || v.pit.reload == 0) {
        v.stop = Stop::HaltedForever;          // IF = 0 or no timer: nothing can wake it
        return;
    }
    wait_until_tsc(pit_next_tick_tsc(v.pit));  // F4-40: sleep until the next timer tick
}
}

uint64_t pit_next_tick_tsc(const VirtualPit& p)
{
    uint64_t done = pit_elapsed(p);
    uint64_t next = (done / p.reload + 1) * p.reload;          // in PIT input periods
    return p.start_tsc + (next / PIT_HZ) * g_tsc_hz + (next % PIT_HZ) * g_tsc_hz / PIT_HZ;
}

void wait_until_tsc(uint64_t deadline)
{
    while (rdtsc() < deadline) {
        asm volatile("sti; hlt; cli");         // the host's 100 Hz tick wakes us
    }
}

void host_calibrate_tsc()
{
    uint64_t t = ticks();
    while (ticks() == t) { asm volatile("hlt"); }
    uint64_t start = rdtsc();
    uint64_t first = ticks();
    while (ticks() < first + 20) { asm volatile("hlt"); }   // 20 ticks of 10 ms
    g_tsc_hz = (rdtsc() - start) * 5;
}

const char* exit_name(uint64_t code)
{
    if (static_cast<uint32_t>(code) == static_cast<uint32_t>(SVM_EXIT_ERR)) {
        code = static_cast<uint64_t>(SVM_EXIT_ERR);
    }
    for (const Name& n : kNames) {
        if (n.code == code) { return n.name; }
    }
    return "other";
}

bool svm_enable()
{
    CpuidRegs ext = cpuid(0x80000001);
    if (!(ext.ecx & (1u << 2))) {                       // APM: CPUID Fn8000_0001 ECX bit 2
        kprintf("hv: this CPU has no SVM (AMD-V)\n");
        return false;
    }
    if (rdmsr(MSR_VM_CR) & (1u << 4)) {
        kprintf("hv: SVM is disabled by firmware (VM_CR.SVMDIS)\n");
        return false;
    }
    CpuidRegs f = cpuid(0x8000000A);                    // APM: SVM revision and features
    kprintf("hv: SVM present; CPUID 0x8000000A: revision %u, %u ASIDs, EDX features %x\n",
            f.eax & 0xFF, f.ebx, f.edx);
    wrmsr(MSR_EFER, rdmsr(MSR_EFER) | EFER_SVME);
    wrmsr(MSR_VM_HSAVE_PA, reinterpret_cast<uint64_t>(g_hsave));   // identity map: VA = PA
    memset(g_iopm, 0xFF, sizeof g_iopm);                // intercept every I/O port
    return true;
}

// A 64-bit guest that shares the host's page tables (no nested paging: F4-41 adds it).
void vcpu_init_long_mode(Vcpu& v, Vmcb* vmcb, uint64_t rip, uint64_t rsp)
{
    memset(vmcb, 0, sizeof *vmcb);
    v.vmcb = vmcb;
    Vmcb& c = *vmcb;
    c.u32(vmcb::INTERCEPT_W3) = w3_bit(SVM_EXIT_INTR) | w3_bit(SVM_EXIT_CPUID) |
                                w3_bit(SVM_EXIT_HLT) | w3_bit(SVM_EXIT_IOIO) |
                                w3_bit(SVM_EXIT_SHUTDOWN);
    c.u32(vmcb::INTERCEPT_W4) = w4_bit(SVM_EXIT_VMRUN) | w4_bit(SVM_EXIT_VMMCALL);
    c.u64(vmcb::IOPM_BASE_PA) = reinterpret_cast<uint64_t>(g_iopm);
    c.u32(vmcb::GUEST_ASID) = 1;
    c.u64(vmcb::VINTR) = 1ull << 24;            // V_INTR_MASKING: host IF guards interrupts

    uint64_t cr0, cr3, cr4;
    asm volatile("mov %%cr0, %0; mov %%cr3, %1; mov %%cr4, %2" : "=r"(cr0), "=r"(cr3), "=r"(cr4));
    struct { uint16_t limit; uint64_t base; } __attribute__((packed)) gdtr;
    asm volatile("sgdt %0" : "=m"(gdtr));
    c.seg(vmcb::CS, 0x08, 0x0A9B, 0xFFFFFFFF, 0);   // 64-bit code: type B, S, P, L, G
    c.seg(vmcb::DS, 0x10, 0x0C93, 0xFFFFFFFF, 0);   // data: type 3, S, P, D/B, G
    c.seg(vmcb::ES, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    c.seg(vmcb::SS, 0x10, 0x0C93, 0xFFFFFFFF, 0);
    c.seg(vmcb::TR, 0, 0x008B, 0xFFFF, 0);          // busy TSS type, present
    c.seg(vmcb::GDTR, 0, 0, gdtr.limit, gdtr.base);
    c.seg(vmcb::IDTR, 0, 0, 0, 0);                  // no IDT: any exception = triple fault
    c.u64(vmcb::EFER) = rdmsr(MSR_EFER);            // LME, LMA and SVME, as the host
    c.u64(vmcb::CR0) = cr0;
    c.u64(vmcb::CR3) = cr3;
    c.u64(vmcb::CR4) = cr4;
    c.u64(vmcb::DR6) = 0xFFFF0FF0;
    c.u64(vmcb::DR7) = 0x400;
    c.u64(vmcb::G_PAT) = rdmsr(0x277);              // the host's PAT
    c.u64(vmcb::RFLAGS) = 0x2;                      // bit 1 is always 1
    c.u64(vmcb::RIP) = rip;
    c.u64(vmcb::RSP) = rsp;
}

void vcpu_run(Vcpu& v, uint64_t max_exits)
{
    v.stop = Stop::Running;
    v.start_tsc = rdtsc();
    while (v.stop == Stop::Running) {
        if (v.entry_hook) { v.entry_hook(v); }
        svm_run(reinterpret_cast<uint64_t>(v.vmcb), &v.regs);
        uint64_t code = v.vmcb->u64(vmcb::EXITCODE);
        if (v.exits < v.trace) {
            kprintf("hv: exit %lu: code %lx info1 %lx info2 %lx rip %lx rax %lx\n", v.exits + 1,
                    code, v.vmcb->u64(vmcb::EXITINFO1), v.vmcb->u64(vmcb::EXITINFO2),
                    v.vmcb->u64(vmcb::RIP), v.vmcb->u64(vmcb::RAX));
        }
        count_exit(v, code);
        switch (code) {
        case SVM_EXIT_INTR:  break;                  // the host took its interrupt already
        case SVM_EXIT_IOIO:  handle_ioio(v); break;
        case SVM_EXIT_CPUID: handle_cpuid(v); break;
        case SVM_EXIT_HLT:   handle_hlt(v); break;
        case SVM_EXIT_VMMCALL:                       // hypercall 0: exit(rbx)
            v.vmcb->u64(vmcb::RIP) += 3;             // VMMCALL is 0F 01 D9
            if (v.vmcb->u64(vmcb::RAX) == 0) { v.guest_exit_code = v.regs.rbx; v.stop = Stop::GuestExit; }
            break;
        case SVM_EXIT_SHUTDOWN:
            kprintf("hv: guest triple fault (SHUTDOWN) at rip %lx; guest destroyed\n",
                    v.vmcb->u64(vmcb::RIP));
            v.stop = Stop::TripleFault;
            break;
        default:
            if (static_cast<uint32_t>(code) == static_cast<uint32_t>(SVM_EXIT_ERR)) {
                // SVM_EXIT_ERR is -1. QEMU 8.2.2 stored only 32 bits of it (R3), so we
                // compare the low half and print the raw 64-bit value.
                kprintf("hv: VMRUN refused the guest state: exit code %lx (SVM_EXIT_ERR, -1)\n",
                        code);
                v.stop = Stop::EntryFailed;
                break;
            }
            if (v.exit_hook && v.exit_hook(v, code)) { break; }
            kprintf("hv: unhandled exit %lx (%s), info1 %lx info2 %lx\n", code, exit_name(code),
                    v.vmcb->u64(vmcb::EXITINFO1), v.vmcb->u64(vmcb::EXITINFO2));
            v.stop = Stop::Unhandled;
        }
        if (v.stop == Stop::Running && v.exits >= max_exits) { v.stop = Stop::TooManyExits; }
    }
    v.end_tsc = rdtsc();
    flush_line(v);
}

void vcpu_report(const Vcpu& v)
{
    static const char* const kStop[] = {"running", "guest exit", "triple fault", "entry failed",
                                        "unhandled exit", "halted forever", "nested page fault",
                                        "too many exits"};
    uint64_t ms = (v.end_tsc - v.start_tsc) / (g_tsc_hz / 1000);
    kprintf("hv: stop reason: %s; guest exit code %lu; %lu exits in %lu ms\n",
            kStop[static_cast<int>(v.stop)], v.guest_exit_code, v.exits, ms);
    kprintf("hv: exits by reason:\n");
    for (const ExitCount& e : v.by_reason) {
        if (e.count == 0) { break; }
        kprintf("    %04lx %-26s %10lu\n", e.code & 0xFFFF, exit_name(e.code), e.count);
    }
    if (v.by_port[0].count != 0) {
        kprintf("hv: IOIO exits by port:\n");
        for (const PortCount& p : v.by_port) {
            if (p.count == 0) { break; }
            kprintf("    port %04x %10lu\n", p.port, p.count);
        }
    }
}
