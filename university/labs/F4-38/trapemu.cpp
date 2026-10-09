// trapemu.cpp - trap-and-emulate on a toy CPU, and the instruction that breaks it.
// The toy CPU has a supervisor and a user mode. Privileged instructions trap in user
// mode; POPIF is "sensitive but not privileged": in user mode it is silently ignored,
// as the chapter explains for x86's POPF. A tiny guest kernel counts in a critical
// section while a timer interrupt handler also changes the counter.
#include <cstdio>
#include <string>
#include <vector>

namespace {
enum Op { LOADI, LOAD, ADDI, STORE, CLI, STI, POPIF, HCALL, LOOP, HLT, IRET };
struct Insn { Op op; int arg; };
const char* kOpName[] = {"LOADI", "LOAD", "ADDI", "STORE", "CLI", "STI", "POPIF", "HCALL",
                         "LOOP", "HLT", "IRET"};

bool privileged(Op op) { return op == CLI || op == STI || op == HLT || op == IRET; }

struct Cpu {
    int acc = 0, pc = 0, loops = 0, saved_pc = 0, saved_acc = 0;
    bool user = false;       // false: supervisor mode
    bool intr_flag = false;  // the real interrupt flag (IF)
};

// The guest kernel: enable interrupts once, then 300 times { disable interrupts;
// counter = counter + 1; enable }. The increment is three instructions, so an interrupt
// in the middle of it loses an update. mem[0] = counter. The timer handler adds 1000.
std::vector<Insn> guest_kernel(Op disable, int arg_off, Op enable, int arg_on)
{
    return {
        {STI, 0},
        {disable, arg_off}, {LOAD, 0}, {ADDI, 1}, {STORE, 0}, {enable, arg_on},
        {LOOP, 300}, {HLT, 0},
        // offset 8: the timer interrupt handler
        {LOAD, 0}, {ADDI, 1000}, {STORE, 0}, {IRET, 0},
    };
}
constexpr int kHandler = 8;
constexpr int kLoopStart = 1;

struct Result { int counter; int traps; int hypercalls; int interrupts; std::string trap_log; };

// mode 0: the guest kernel runs natively in supervisor mode (the reference).
// mode 1: the guest kernel runs deprivileged in user mode under a trap-and-emulate monitor
//         that keeps a virtual interrupt flag (vIF) and delivers virtual interrupts.
Result run(const std::vector<Insn>& code, int mode)
{
    std::vector<int> mem(4, 0);
    Cpu c;
    c.user = (mode == 1);
    bool vif = false;            // the monitor's virtual IF for the guest
    bool in_handler = false;
    Result r{0, 0, 0, 0, ""};
    for (long step = 0; step < 100000; ++step) {
        // A timer tick every 7 instructions: interrupt if (virtual) IF allows it.
        bool can_interrupt = (mode == 0) ? c.intr_flag : vif;
        if (step % 7 == 6 && can_interrupt && !in_handler) {
            c.saved_pc = c.pc; c.saved_acc = c.acc; c.pc = kHandler; in_handler = true;
            ++r.interrupts;
            if (mode == 0) { c.intr_flag = false; } else { vif = false; }
        }
        const Insn in = code[c.pc];
        if (c.user && privileged(in.op)) {       // trap to the monitor, which emulates
            ++r.traps;
            if (r.trap_log.size() < 60) { r.trap_log += std::string(kOpName[in.op]) + " "; }
            if (in.op == CLI) { vif = false; }
            if (in.op == STI) { vif = true; }
            if (in.op == IRET) { c.pc = c.saved_pc; c.acc = c.saved_acc; in_handler = false;
                                 vif = true; continue; }
            if (in.op == HLT) { break; }
            ++c.pc;
            continue;
        }
        switch (in.op) {
        case LOADI: c.acc = in.arg; break;
        case LOAD: c.acc = mem[in.arg]; break;
        case ADDI: c.acc += in.arg; break;
        case STORE: mem[in.arg] = c.acc; break;
        case CLI: c.intr_flag = false; break;
        case STI: c.intr_flag = true; break;
        case POPIF:                                // sensitive, NOT privileged:
            if (!c.user) { c.intr_flag = (in.arg != 0); }   // ignored in user mode, no trap
            break;
        case HCALL:                                // paravirtual: ask the monitor directly
            ++r.hypercalls;
            vif = (in.arg != 0);
            break;
        case LOOP: if (++c.loops < in.arg) { c.pc = kLoopStart; continue; } break;
        case HLT: r.counter = mem[0]; return r;
        case IRET: c.pc = c.saved_pc; c.acc = c.saved_acc; in_handler = false;
                   c.intr_flag = true; continue;
        }
        ++c.pc;
    }
    r.counter = mem[0];
    return r;
}

void show(const char* what, const Result& r)
{
    int expected = 300 + 1000 * r.interrupts;
    std::printf("%-44s counter %6d, expected %6d (%d interrupts) %s\n", what, r.counter,
                expected, r.interrupts, r.counter == expected ? "OK" : "LOST UPDATES");
    std::printf("%-44s traps %d, hypercalls %d; first traps: %s\n", "", r.traps,
                r.hypercalls, r.trap_log.c_str());
}
}

int main()
{
    show("1 native, CLI/STI", run(guest_kernel(CLI, 0, STI, 0), 0));
    show("2 trap-and-emulate, CLI/STI", run(guest_kernel(CLI, 0, STI, 0), 1));
    show("3 native, POPIF 0/1", run(guest_kernel(POPIF, 0, POPIF, 1), 0));
    show("4 trap-and-emulate, POPIF 0/1", run(guest_kernel(POPIF, 0, POPIF, 1), 1));
    show("5 paravirtual: POPIF replaced by HCALL", run(guest_kernel(HCALL, 0, HCALL, 1), 1));
    return 0;
}
