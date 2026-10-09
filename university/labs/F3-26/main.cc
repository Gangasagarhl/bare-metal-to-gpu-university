// main.cc - kmain: bring the boot CPU up, then run the test named on the command line
// ("-append test=threads"). Each test prints its result; QEMU's exit status says
// pass (1) or fail (3) through the isa-debug-exit device.
#include "hooks.h"
#include "thread.h"

extern "C" uint32_t mb_magic, mb_info;
extern "C" void (*__init_array_start[])();
extern "C" void (*__init_array_end[])();

namespace k {
bool test_threads();        // F3-26 (B9)
bool test_stack_overflow(); // F3-26 forensic
bool test_sync();           // F3-27 (B10)
bool test_irq_lock();       // F3-27 forensic
bool test_smp();            // F3-28 (B11)
bool test_abba();           // F3-28 forensic
bool test_user();           // F3-29 (B12)
bool test_proc();           // F3-30 (B13)
bool test_execloop();       // F3-30 (B13)
bool test_fuzz();           // F3-30 (B13)

namespace {
struct Test {
    const char* name;
    bool (*fn)();
};
const Test kTests[] = {
    {"threads", test_threads}, {"stackoverflow", test_stack_overflow}, {"sync", test_sync},
    {"irqlock", test_irq_lock}, {"smp", test_smp},       {"abba", test_abba},
    {"user", test_user},       {"proc", test_proc},      {"execloop", test_execloop},
    {"fuzz", test_fuzz},
};
}  // namespace
}  // namespace k

extern "C" [[noreturn]] void kmain()
{
    using namespace k;
    Cpu& c0 = cpu_by_index(0);
    idt_init();
    cpu_setup(c0, 0, 0);
    for (auto* f = __init_array_start; f != __init_array_end; ++f) {
        (*f)();   // global constructors, if any
    }
    console_init();
    kprintf("\nOS303 teaching kernel: cpu0 in 64-bit mode\n");
    if (mb_magic != 0x2BADB002) {
        panic("not started by a Multiboot loader (magic %x)", mb_magic);
    }
    mem_init(mb_info);
    kprintf("command line: '%s'\n", boot_cmdline());
    sched_init_cpu(c0, "idle0");
    c0.online = true;
    pic_disable();
    lapic_init_cpu();
    c0.apic_id = lapic_id();
    lapic_calibrate();
    lockup_set_limit_ms(2000, tsc_per_ms());
    syscall_init_cpu();
    lapic_timer_start(kTickHz);
    irq_enable();
    smp_start_aps();

    const char* name = boot_arg("test");
    for (const Test& t : kTests) {
        if (name != nullptr && str_eq(name, t.name)) {
            kprintf("=== test %s ===\n", t.name);
            bool ok = t.fn();
            kprintf("=== test %s: %s ===\n", t.name, ok ? "PASS" : "FAIL");
            qemu_exit(ok ? 0 : 1);
        }
    }
    panic("no such test: '%s'", name ? name : "(none given)");
}
