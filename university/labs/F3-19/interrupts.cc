// interrupts.cc - F3-19: IDT gates, the C++ side of every interrupt, exception reports.
#include "interrupts.h"
#include "arch.h"
#include "gdt.h"
#include "kprint.h"
#include "panic.h"

extern "C" char isr_stubs[];            // isr.S: stub n at isr_stubs + 16 * n
extern "C" {
volatile uint64_t g_resume_rip = 0;
}
void (*g_double_fault_hook)(const InterruptFrame&) = nullptr;   // set by tests

namespace {

struct [[gnu::packed]] Gate {
    uint16_t offset_low;
    uint16_t selector;
    uint8_t ist;                        // bits 0-2: IST slot, 0 = keep the current stack
    uint8_t type_attr;                  // present | DPL | type (0xE = 64-bit interrupt gate)
    uint16_t offset_mid;
    uint32_t offset_high;
    uint32_t reserved;
};
static_assert(sizeof(Gate) == 16, "64-bit IDT gates are 16 bytes");

alignas(16) Gate g_idt[256];
IrqHandler g_handlers[256];

Gate make_gate(uint64_t handler, uint8_t ist, uint8_t dpl)
{
    Gate g{};
    g.offset_low = handler & 0xFFFF;
    g.selector = gdt::kKernelCode;
    g.ist = ist;
    g.type_attr = static_cast<uint8_t>(0x80 | (dpl << 5) | 0xE);
    g.offset_mid = (handler >> 16) & 0xFFFF;
    g.offset_high = static_cast<uint32_t>(handler >> 32);
    return g;
}

const char* const kNames[32] = {
    "#DE divide error", "#DB debug", "NMI", "#BP breakpoint", "#OF overflow",
    "#BR bound range exceeded", "#UD invalid opcode", "#NM device not available",
    "#DF double fault", "coprocessor segment overrun", "#TS invalid TSS",
    "#NP segment not present", "#SS stack-segment fault", "#GP general protection",
    "#PF page fault", "reserved 15", "#MF x87 floating-point error", "#AC alignment check",
    "#MC machine check", "#XM SIMD floating-point", "#VE virtualization",
    "#CP control protection", "reserved 22", "reserved 23", "reserved 24", "reserved 25",
    "reserved 26", "reserved 27", "reserved 28", "reserved 29", "reserved 30", "reserved 31"};

void report(const InterruptFrame& f)
{
    kprintf("EXCEPTION %lu %s, error code 0x%lx, at rip %p\n", f.vector,
            exception_name(f.vector), f.error_code, reinterpret_cast<void*>(f.rip));
    if (f.vector == 14) {
        uint64_t e = f.error_code;
        kprintf("  page fault: cr2 %p: %s, %s, %s mode%s%s\n", reinterpret_cast<void*>(arch::read_cr2()),
                (e & 1) ? "protection violation" : "page not present",
                (e & 2) ? "write" : "read", (e & 4) ? "user" : "supervisor",
                (e & 8) ? ", reserved bit set" : "", (e & 16) ? ", instruction fetch" : "");
    }
}

} // namespace

const char* exception_name(uint64_t vector)
{
    return vector < 32 ? kNames[vector] : "external interrupt";
}

void dump_frame(const InterruptFrame& f)
{
    kprintf("  rax %016lx rbx %016lx rcx %016lx rdx %016lx\n", f.rax, f.rbx, f.rcx, f.rdx);
    kprintf("  rsi %016lx rdi %016lx rbp %016lx rsp %016lx\n", f.rsi, f.rdi, f.rbp, f.rsp);
    kprintf("  r8  %016lx r9  %016lx r10 %016lx r11 %016lx\n", f.r8, f.r9, f.r10, f.r11);
    kprintf("  r12 %016lx r13 %016lx r14 %016lx r15 %016lx\n", f.r12, f.r13, f.r14, f.r15);
    kprintf("  rip %016lx cs %04lx rflags %08lx ss %04lx\n", f.rip, f.cs, f.rflags, f.ss);
    kprintf("  cr0 %016lx cr2 %016lx cr3 %016lx cr4 %016lx\n", arch::read_cr0(),
            arch::read_cr2(), arch::read_cr3(), arch::read_cr4());
}

extern "C" void interrupt_dispatch(InterruptFrame* frame)
{
    InterruptFrame& f = *frame;
    if (f.vector >= 32) {
        if (g_handlers[f.vector] != nullptr) {
            g_handlers[f.vector](f);
        } else {
            kprintf("interrupt: no handler for vector %lu\n", f.vector);
        }
        return;
    }
    bool recoverable = f.vector == 3 || (g_resume_rip != 0 && f.vector != 8);
    if (!recoverable) {
        kprint_panic_mode();            // from here on we are panicking: never wait for a lock
    }
    report(f);
    if (f.vector == 3) {                // #BP is a trap: rip already points after int3
        kprintf("  breakpoint handled, resuming at %p\n", reinterpret_cast<void*>(f.rip));
        return;
    }
    if (g_resume_rip != 0 && f.vector != 8) {
        f.rip = g_resume_rip;           // test fixup: continue after the faulting code
        g_resume_rip = 0;
        kprintf("  test fixup: resuming at %p\n", reinterpret_cast<void*>(f.rip));
        return;
    }
    dump_frame(f);
    if (f.vector == 8 && g_double_fault_hook != nullptr) {
        g_double_fault_hook(f);
    }
    if (f.vector == 8) {
        uint64_t rsp_now;
        asm volatile("mov %%rsp, %0" : "=r"(rsp_now));
        kprintf("  double fault handler running on stack %p (IST1 top %p)\n",
                reinterpret_cast<void*>(rsp_now), reinterpret_cast<void*>(gdt::tss().ist[0]));
    }
    PANIC("unhandled exception %lu (%s)", f.vector, exception_name(f.vector));
}

namespace idt {

void init(bool use_ist)
{
    for (int v = 0; v < 256; ++v) {
        uint8_t ist = 0;
        if (use_ist) {
            ist = v == 8 ? gdt::kIstDoubleFault : v == 2 ? gdt::kIstNmi : v == 18 ? gdt::kIstMachineCheck : 0;
        }
        g_idt[v] = make_gate(reinterpret_cast<uint64_t>(isr_stubs + 16 * v), ist, 0);
    }
    struct [[gnu::packed]] {
        uint16_t limit;
        uint64_t base;
    } p{sizeof(g_idt) - 1, reinterpret_cast<uint64_t>(g_idt)};
    asm volatile("lidt %0" : : "m"(p) : "memory");
    kprintf("idt: 256 gates at %p\n", static_cast<void*>(g_idt));
}

void dump_gate(uint8_t vector)
{
    const auto* q = reinterpret_cast<const uint64_t*>(&g_idt[vector]);
    kprintf("idt[%u] raw: low %016lx high %016lx\n", unsigned{vector}, q[0], q[1]);
}

void set_handler(uint8_t vector, IrqHandler h)
{
    g_handlers[vector] = h;
}

} // namespace idt
