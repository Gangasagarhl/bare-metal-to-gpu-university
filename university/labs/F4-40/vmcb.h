// vmcb.h - the AMD-V virtual machine control block (VMCB) as offsets into one 4 KiB page.
// Exit codes come from Linux's <asm/svm.h> (linux-libc-dev 6.8.0, opened in this build).
// The field OFFSETS and intercept BIT positions are NOT in that header: they were written
// from memory of the AMD64 Architecture Programmer's Manual, Volume 2, appendix "Layout of
// VMCB" (title only, not opened in this build). The F4-40/F4-41 runs prove that QEMU 8.2.2's
// SVM model agrees with them, nothing more: see the chapter's unverified box.
#pragma once
#include <stdint.h>
#include <asm/svm.h>

namespace vmcb {
// ---- control area (offset 0x000 - 0x3FF) ----
constexpr uint32_t INTERCEPT_CR   = 0x000;  // u32: bits 0-15 CR reads, 16-31 CR writes
constexpr uint32_t INTERCEPT_EXC  = 0x008;  // u32: bit n intercepts exception vector n
constexpr uint32_t INTERCEPT_W3   = 0x00C;  // u32: bit n intercepts exit code 0x060 + n
constexpr uint32_t INTERCEPT_W4   = 0x010;  // u32: bit n intercepts exit code 0x080 + n
constexpr uint32_t IOPM_BASE_PA   = 0x040;  // u64: physical address of the I/O bitmap
constexpr uint32_t MSRPM_BASE_PA  = 0x048;  // u64: physical address of the MSR bitmap
constexpr uint32_t GUEST_ASID     = 0x058;  // u32: address-space id, must not be 0
constexpr uint32_t TLB_CONTROL    = 0x05C;  // u8
constexpr uint32_t VINTR          = 0x060;  // u64: bit 24 V_INTR_MASKING
constexpr uint32_t EXITCODE       = 0x070;  // u64: why the guest stopped (SVM_EXIT_*)
constexpr uint32_t EXITINFO1      = 0x078;  // u64: details, meaning depends on EXITCODE
constexpr uint32_t EXITINFO2      = 0x080;  // u64
constexpr uint32_t EXITINTINFO    = 0x088;  // u64: event that was being delivered
constexpr uint32_t NP_ENABLE      = 0x090;  // u64: bit 0 nested paging on (F4-41)
constexpr uint32_t EVENTINJ       = 0x0A8;  // u64: event to inject at the next VMRUN
constexpr uint32_t N_CR3          = 0x0B0;  // u64: nested page table root (F4-41)
// ---- state save area (offset 0x400 - ) ----
constexpr uint32_t ES = 0x400, CS = 0x410, SS = 0x420, DS = 0x430, FS = 0x440, GS = 0x450;
constexpr uint32_t GDTR = 0x460, LDTR = 0x470, IDTR = 0x480, TR = 0x490;
// each segment: u16 selector (+0), u16 attributes (+2), u32 limit (+4), u64 base (+8)
constexpr uint32_t CPL    = 0x4CB;          // u8
constexpr uint32_t EFER   = 0x4D0;
constexpr uint32_t CR4    = 0x548, CR3 = 0x550, CR0 = 0x558, DR7 = 0x560, DR6 = 0x568;
constexpr uint32_t RFLAGS = 0x570, RIP = 0x578, RSP = 0x5D8, RAX = 0x5F8;
constexpr uint32_t CR2    = 0x640, G_PAT = 0x668;
}

// Intercept bit for exit code c: the APM numbers the intercept bits in the same order as
// the exit codes (word 3 starts at SVM_EXIT_INTR, word 4 at SVM_EXIT_VMRUN).
constexpr uint32_t w3_bit(uint32_t exit_code) { return 1u << (exit_code - SVM_EXIT_INTR); }
constexpr uint32_t w4_bit(uint32_t exit_code) { return 1u << (exit_code - SVM_EXIT_VMRUN); }

struct alignas(4096) Vmcb {
    uint8_t bytes[4096];
    template <typename T> T& at(uint32_t off) { return *reinterpret_cast<T*>(bytes + off); }
    uint64_t& u64(uint32_t off) { return at<uint64_t>(off); }
    uint32_t& u32(uint32_t off) { return at<uint32_t>(off); }
    void seg(uint32_t off, uint16_t sel, uint16_t attrib, uint32_t limit, uint64_t base)
    {
        at<uint16_t>(off) = sel;
        at<uint16_t>(off + 2) = attrib;
        at<uint32_t>(off + 4) = limit;
        at<uint64_t>(off + 8) = base;
    }
};
static_assert(sizeof(Vmcb) == 4096);
