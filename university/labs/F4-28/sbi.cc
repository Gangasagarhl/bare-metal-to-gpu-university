// sbi.cc - F4-28: SBI shutdown and arch::qemu_exit for RISC-V.
#include "sbi.h"
#include "arch.h"
#include "kprint.h"

namespace sbi {

[[noreturn]] void shutdown()
{
    Ret r = call(kSrst, 0, 0, 0);   // system_reset(type 0 = shutdown, reason 0 = no reason)
    kprintf("SBI system reset returned (error %ld): halting this hart instead\n", r.error);
    arch::halt_forever();           // only reached if the firmware could not shut down
}

} // namespace sbi

namespace arch {

[[noreturn]] void qemu_exit(uint8_t code)
{
    kprintf("exit: %s (code 0x%x); SBI system reset (shutdown)\n", code == kExitPass ? "pass" : "FAIL", code);
    sbi::shutdown();
}

} // namespace arch
