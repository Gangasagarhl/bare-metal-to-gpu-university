// F1-24 Listing 1: run a U16 program one instruction at a time and print what every part
// of the single-cycle datapath did: fetch, decode, register read, ALU, memory, write back.
#include <cstdio>
#include <iostream>
#include "../F1-23/u16.h"

int main()
{
    const u16::Program p = u16::assemble(std::cin);
    if (!p.errors.empty()) {
        std::cout << p.errors.front() << '\n';
        return 1;
    }
    u16::Machine m(p);
    std::cout << "cyc  PC  word    instruction        A(rs)  B     ALU    memory      write back"
                 "    next PC\n";
    for (int cycle = 1; !m.halted && cycle <= 60; ++cycle) {
        const u16::Step s = m.step();
        char mem[32] = "-";
        if (s.c.memRead) {
            std::snprintf(mem, sizeof mem, "read [%u]", static_cast<unsigned>(s.aluOut));
        } else if (s.c.memWrite) {
            std::snprintf(mem, sizeof mem, "write [%u]", static_cast<unsigned>(s.aluOut));
        }
        char wb[32] = "-";
        if (s.wrote) {
            std::snprintf(wb, sizeof wb, "r%u = %d", s.f.rd, static_cast<std::int16_t>(s.value));
        } else if (s.c.out) {
            std::snprintf(wb, sizeof wb, "OUT %d", static_cast<std::int16_t>(s.value));
        }
        std::printf("%3d %3d  0x%04X  %-17s %5d %5d %6d    %-10s  %-12s %3d%s\n", cycle, s.pc,
                    static_cast<unsigned>(s.word), u16::disasm(s.word).c_str(),
                    static_cast<std::int16_t>(s.a), static_cast<std::int16_t>(s.b),
                    static_cast<std::int16_t>(s.aluOut), mem, wb, s.nextPc,
                    s.taken ? "  (branch taken)" : "");
    }
    std::cout << "registers:";
    for (std::size_t r = 0; r < m.reg.size(); ++r) {
        std::cout << " r" << r << '=' << static_cast<std::int16_t>(m.reg[r]);
    }
    std::cout << '\n';
    return m.fault.empty() ? 0 : 1;
}
