// psci.cc - F4-24: the PSCI conduit (HVC or SMC, chosen by the devicetree) and arch::qemu_exit.
#include "psci.h"
#include "arch.h"
#include "kprint.h"

namespace {
enum class Conduit { None, Hvc, Smc };
Conduit g_conduit = Conduit::None;
} // namespace

namespace psci {

bool init(const fdt::Blob& dt)
{
    fdt::Node n;
    fdt::Prop p;
    if (!dt.find_path("/psci", &n) || !dt.get_prop(n, "method", &p)) {
        return false;
    }
    const char* m = reinterpret_cast<const char*>(p.data);
    g_conduit = fdt::streq(m, "hvc") ? Conduit::Hvc : fdt::streq(m, "smc") ? Conduit::Smc : Conduit::None;
    return g_conduit != Conduit::None;
}

const char* method()
{
    return g_conduit == Conduit::Hvc ? "hvc" : g_conduit == Conduit::Smc ? "smc" : "none";
}

// SMC Calling Convention: function ID in w0/x0, arguments in x1-x3, result in x0.
int64_t call(uint64_t fn, uint64_t a1, uint64_t a2, uint64_t a3)
{
    register uint64_t x0 asm("x0") = fn;
    register uint64_t x1 asm("x1") = a1;
    register uint64_t x2 asm("x2") = a2;
    register uint64_t x3 asm("x3") = a3;
    if (g_conduit == Conduit::Hvc) {
        asm volatile("hvc #0" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3) : : "memory");
    } else if (g_conduit == Conduit::Smc) {
        asm volatile("smc #0" : "+r"(x0), "+r"(x1), "+r"(x2), "+r"(x3) : : "memory");
    } else {
        return -1;   // PSCI "not supported"
    }
    return static_cast<int64_t>(x0);
}

[[noreturn]] void system_off()
{
    call(kSystemOff);
    arch::halt_forever();   // only reached if the firmware refused
}

} // namespace psci

namespace arch {

[[noreturn]] void qemu_exit(uint8_t code)
{
    kprintf("exit: %s (code 0x%x); PSCI SYSTEM_OFF through %s\n",
            code == kExitPass ? "pass" : "FAIL", code, psci::method());
    psci::system_off();
}

} // namespace arch
