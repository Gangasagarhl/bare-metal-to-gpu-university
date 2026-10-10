// hv.h - the DR404 tiny hypervisor (AMD-V / SVM): one virtual CPU, exits counted by reason.
#pragma once
#include <stdint.h>
#include "vmcb.h"

struct GuestRegs {   // the general registers VMRUN does not save (order used by vmrun.S)
    uint64_t rbx, rcx, rdx, rsi, rdi, rbp, r8, r9, r10, r11, r12, r13, r14, r15;
};

struct ExitCount { uint64_t code; uint64_t count; };
struct PortCount { uint16_t port; uint64_t count; };

struct VirtualPit {             // 8254 channel 0, timed by the host's TSC
    uint16_t reload = 0;        // 0 = not programmed yet
    uint16_t latched = 0;
    bool have_latch = false;
    bool read_high = false;     // lo/hi byte access: which byte comes next
    bool write_high = false;
    uint64_t start_tsc = 0;
};

struct Vcpu;
using IoHook = bool (*)(Vcpu&, uint16_t port, bool in, uint32_t& value);  // true = handled
using ExitHook = bool (*)(Vcpu&, uint64_t code);   // true = handled (F4-41: nested faults)
using EntryHook = void (*)(Vcpu&);                  // runs before every VMRUN (F4-41)

enum class Stop { Running, GuestExit, TripleFault, EntryFailed, Unhandled, HaltedForever,
                  NestedFault, TooManyExits };

struct Vcpu {
    Vmcb* vmcb = nullptr;
    GuestRegs regs{};
    Stop stop = Stop::Running;
    uint64_t guest_exit_code = 0;
    uint64_t exits = 0;
    ExitCount by_reason[24]{};
    PortCount by_port[16]{};
    VirtualPit pit{};
    char line[120]{};           // UART model: the guest's current output line
    uint32_t line_len = 0;
    IoHook io_hook = nullptr;
    ExitHook exit_hook = nullptr;
    EntryHook entry_hook = nullptr;
    bool (*halt_hook)(Vcpu&) = nullptr;   // F4-41: wake the guest with an interrupt
    uint64_t start_tsc = 0, end_tsc = 0;
    uint64_t trace = 0;         // print the raw fields of the first `trace` exits
};

extern uint64_t g_tsc_hz;                    // host TSC rate, measured at boot
void host_calibrate_tsc();                    // needs the host's PIT tick running
bool svm_enable();                            // CPUID check, EFER.SVME, host-save area
void vcpu_init_long_mode(Vcpu& v, Vmcb* vmcb, uint64_t rip, uint64_t rsp);
void vcpu_run(Vcpu& v, uint64_t max_exits);   // until the guest stops, for any reason
void vcpu_report(const Vcpu& v);              // exit counts by reason and by port
const char* exit_name(uint64_t code);
uint64_t pit_next_tick_tsc(const VirtualPit& p);
void wait_until_tsc(uint64_t deadline);       // the host halts until then

extern "C" void svm_run(uint64_t vmcb_pa, GuestRegs* regs);
