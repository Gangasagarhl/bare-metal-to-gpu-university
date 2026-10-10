// F1-27 Listing 1: the same U16 program through the pipeline model twice: without
// forwarding (wait for write back) and with forwarding. Program on standard input.
//   hazards [rows]   (default 10 diagram rows)
#include <cstdlib>
#include <iostream>
#include "../F1-26/pipe.h"

int main(int argc, char** argv)
{
    const std::size_t rows = argc > 1 ? static_cast<std::size_t>(std::atoi(argv[1])) : 10;
    const u16::Program p = u16::assemble(std::cin);
    if (!p.errors.empty()) {
        std::cout << p.errors.front() << '\n';
        return 1;
    }
    std::cout << "== without forwarding: a reader waits until the writer's WB cycle ==\n";
    const pipe::Result slow = pipe::schedule(p, pipe::Config{true, false, true});
    pipe::draw(slow, rows);
    pipe::summary(slow);
    std::cout << "== with forwarding: results go straight from EX or MEM to the next EX ==\n";
    const pipe::Result fast = pipe::schedule(p, pipe::Config{true, true, true});
    pipe::draw(fast, rows);
    pipe::summary(fast);
    return 0;
}
