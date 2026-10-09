// b6_main.cc - F3-23 test kernel for milestone B6 (consoles, logging, panic screen).
//   (none)              banner, all log levels, enough lines to scroll; waits for a screendump
//   -DB6_PANIC_TEST     takes the log lock, then faults: the panic must still print and exit
//   -DPANIC_KEEPS_LOCK  (with B6_PANIC_TEST) the forensic kernel: the panic path waits for the lock
#include <cstdint>
#include "arch.h"
#include "gdt.h"
#include "interrupts.h"
#include "kprint.h"
#include "log.h"
#include "multiboot.h"
#include "paging.h"
#include "panic.h"
#include "pmm.h"
#include "serial.h"
#include "smbios.h"

extern "C" void kmain(uint32_t magic, uint32_t info_phys)
{
    serial::init();
    KASSERT(magic == mb::kBootMagic);
    gdt::init(true);
    idt::init(true);
    pmm::init(static_cast<const mb::Info*>(pmm::phys_to_virt(info_phys)), true);
    KASSERT(paging::build_kernel_space(info_phys));
    log_init(true);
    klog(Level::Info, "OS302 kernel, chapter F3-23 build");
    smbios::print_banner();
#if defined(B6_PANIC_TEST)
    klog(Level::Warn, "test: holding the log lock, then executing ud2");
    log_lock_acquire();
    asm volatile("ud2");
    kprintf("not reached\n");
#else
    klog(Level::Debug, "a debug line (grey)");
    klog(Level::Info, "an information line (light grey)");
    klog(Level::Warn, "a warning line (yellow)");
    klog(Level::Error, "an error line (red)");
    for (int i = 1; i <= 50; ++i) {
        klog(Level::Info, "scroll test line %d of 50: the quick brown fox jumps over the lazy dog", i);
    }
    kprintf("B6 screen ready; waiting for the screendump\n");
    for (;;) {
        arch::hlt();
    }
#endif
}
