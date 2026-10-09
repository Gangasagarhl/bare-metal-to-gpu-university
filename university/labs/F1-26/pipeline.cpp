// F1-26 Listing 2: draw the pipeline diagram of a U16 program and count its cycles.
//   pipeline [ideal|stall|forward] [rows]   (program on standard input; default: ideal 12)
#include <cstdlib>
#include <iostream>
#include <string>
#include "pipe.h"

int main(int argc, char** argv)
{
    const std::string mode = argc > 1 ? argv[1] : "ideal";
    const std::size_t rows = argc > 2 ? static_cast<std::size_t>(std::atoi(argv[2])) : 12;
    pipe::Config cfg{false, false, false};               // ideal: nothing ever waits
    if (mode == "stall") {
        cfg = pipe::Config{true, false, true};           // hazards, no forwarding
    } else if (mode == "forward") {
        cfg = pipe::Config{true, true, true};            // hazards, with forwarding
    } else if (mode != "ideal") {
        std::cout << "unknown mode " << mode << '\n';
        return 2;
    }
    const u16::Program p = u16::assemble(std::cin);
    if (!p.errors.empty()) {
        std::cout << p.errors.front() << '\n';
        return 1;
    }
    const pipe::Result r = pipe::schedule(p, cfg);
    std::cout << "mode: " << mode << '\n';
    pipe::draw(r, rows);
    pipe::summary(r);
    std::cout << "single-cycle machine: " << r.instructions << " cycles (one long cycle each)\n";
    return 0;
}
