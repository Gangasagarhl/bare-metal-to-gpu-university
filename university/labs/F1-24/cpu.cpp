// F1-24 Listing 2: the U16 computer. Load a program (assembly or raw machine words),
// run it until HALT, print what it sent to the output port and how many instructions ran.
#include <iostream>
#include "../F1-23/u16.h"

int main()
{
    const u16::Program p = u16::assemble(std::cin);
    for (const std::string& e : p.errors) {
        std::cout << "error: " << e << '\n';
    }
    u16::Machine m(p);
    long executed = 0;
    while (!m.halted && executed < 100000) {
        m.step();
        ++executed;
    }
    for (const std::uint16_t v : m.output) {
        std::cout << "out: " << static_cast<std::int16_t>(v) << '\n';
    }
    std::cout << "halted: " << (m.halted ? "yes" : "no (step limit)") << ", instructions executed: "
              << executed << '\n';
    if (!m.fault.empty()) {
        std::cout << "fault: " << m.fault << '\n';
    }
    return (p.errors.empty() && m.fault.empty()) ? 0 : 1;
}
