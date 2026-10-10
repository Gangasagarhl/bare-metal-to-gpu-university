// panic.cc - F3-18: interrupts off, one clear line on every output, then tell QEMU "fail".
#include "panic.h"
#include "arch.h"
#include "kprint.h"

[[noreturn]] void panic_at(const char* file, int line, const char* fmt, ...)
{
    arch::cli();                     // nothing may interrupt the report
    kprint_panic_mode();             // print even if the interrupted code held the output lock
    kprintf("\nPANIC at %s:%d: ", file, line);
    va_list ap;
    va_start(ap, fmt);
    kvprintf(fmt, ap);
    va_end(ap);
    kprintf("\n");
    arch::qemu_exit(arch::kExitFail);   // under QEMU: exit status 35; on hardware: halt
}
