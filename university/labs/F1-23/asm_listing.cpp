// F1-23 Listing 2: assemble a U16 program, show every machine word field by field, run it.
#include <bitset>
#include <cstdio>
#include <iostream>
#include <string>
#include "u16.h"

int main()
{
    const u16::Program p = u16::assemble(std::cin);
    for (const std::string& e : p.errors) {
        std::cout << "error: " << e << '\n';
    }
    std::cout << "addr  word    op   rd  rs  rest    meaning\n";
    for (std::size_t a = 0; a < p.code.size(); ++a) {
        const std::uint16_t w = p.code[a];
        const std::string bits = std::bitset<16>(w).to_string();
        std::printf("%4zu  0x%04X  %s %s %s %s  %s\n", a, static_cast<unsigned>(w),
                    bits.substr(0, 4).c_str(), bits.substr(4, 3).c_str(),
                    bits.substr(7, 3).c_str(), bits.substr(10, 6).c_str(),
                    u16::disasm(w).c_str());
    }
    u16::Machine m(p);
    long steps = 0;
    while (!m.halted && steps < 10000) {
        m.step();
        ++steps;
    }
    for (const std::uint16_t v : m.output) {
        std::cout << "out: " << static_cast<std::int16_t>(v) << '\n';
    }
    std::cout << "instructions executed: " << steps << '\n';
    if (!m.halted) {
        std::cout << "stopped: step limit of 10000 reached (the program did not halt)\n";
    }
    if (!m.fault.empty()) {
        std::cout << "fault: " << m.fault << '\n';
    }
    return p.errors.empty() ? 0 : 1;
}
