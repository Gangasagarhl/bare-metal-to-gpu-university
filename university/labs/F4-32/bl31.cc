// bl31.cc - F4-32 stage 2 ("BL31", the EL3 runtime). It stays resident in secure RAM after the
// kernel starts and answers the kernel's SMC calls. It implements two calls of the Arm Power
// State Coordination Interface (PSCI_VERSION and SYSTEM_OFF); everything else is "not supported".
#include "../F4-31/kbase.h"

extern "C" char el3_vectors[];
extern "C" [[noreturn]] void enter_bl33(uint64_t entry, uint64_t dtb, uint64_t scr, uint64_t spsr);

namespace {
constexpr uint64_t kUart = 0x09000000;
void putc_bl31(char c)
{
    while ((k::rd32(kUart + 0x18) & (1u << 5)) != 0) {
    }
    k::wr32(kUart, static_cast<uint8_t>(c));
}
// PSCI function identifiers and return codes (SMC32 calling convention) after the Arm PSCI
// specification (title only, pending verification).
constexpr uint32_t kPsciVersion = 0x84000000;
constexpr uint32_t kPsciSystemOff = 0x84000008;
constexpr int64_t kPsciNotSupported = -1;

#ifndef SCR_VALUE
// SCR_EL3: NS (bit 0) = lower levels are non-secure; bits 5:4 are RES1; RW (bit 10) = the next
// lower level runs AArch64 (Arm ARM, SCR_EL3 description - title only, pending verification).
#define SCR_VALUE ((1u << 0) | (3u << 4) | (1u << 10))
#endif
constexpr uint64_t kSpsrEl1h = 0x3c5;          // return to EL1 using SP_EL1, interrupts masked
}  // namespace

extern "C" int64_t smc_dispatch(uint64_t fid, uint64_t, uint64_t, uint64_t, uint64_t esr)
{
    switch (static_cast<uint32_t>(fid)) {
    case kPsciVersion:
        k::printf("BL31: SMC PSCI_VERSION from the normal world -> 1.0\n");
        return 0x00010000;                     // major 1, minor 0
    case kPsciSystemOff:
        k::printf("BL31: SMC SYSTEM_OFF from the normal world -> powering off\n");
        k::exit(0);
    default:
        k::printf("BL31: unknown SMC 0x%lx (ESR_EL3 0x%lx) -> NOT_SUPPORTED\n", fid, esr);
        return kPsciNotSupported;
    }
}

extern "C" [[noreturn]] void el3_report(uint64_t idx, uint64_t esr, uint64_t elr)
{
    k::printf("BL31: unexpected exception, vector %lu ESR_EL3=0x%lx ELR_EL3=0x%lx\n", idx, esr, elr);
    k::exit(7);
}

extern "C" void stage_main(uint64_t bl33_entry, uint64_t dtb)
{
    k::set_console(putc_bl31);
    asm volatile("msr vbar_el3, %0; isb" : : "r"(el3_vectors));
    k::printf("BL31: resident at EL3, VBAR_EL3 = %p; CNTFRQ_EL0 = %lu Hz (reset value kept)\n",
              static_cast<void*>(el3_vectors), k::counter_freq());
    k::printf("BL31: SCR_EL3 = 0x%x, entering BL33 at 0x%lx (EL1h) with x0 = DTB 0x%lx\n",
              static_cast<unsigned>(SCR_VALUE), bl33_entry, dtb);
    enter_bl33(bl33_entry, dtb, SCR_VALUE, kSpsrEl1h);
}
