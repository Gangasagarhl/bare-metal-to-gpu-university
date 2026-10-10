// pipe_exam_start.cpp - HW202 practical (P): the STARTING FILE handed to candidates.
// A five-stage pipeline timing model written from the rules of F1-26 and F1-27, using only
// the U16 machine of u16.h. Four functions marked TODO are missing: they are the hazard rules.
// Complete them and nothing else. Do not include the course's pipe.h (reading it is allowed,
// including it is not: the point is to write the rules yourself).
//   build: g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -g -fsanitize=address,undefined
//          pipe_exam_start.cpp -o pipe_exam_start
//   run:   ./pipe_exam_start 16 < pipe_exam_start.in
// As handed out it builds and runs, prints wrong numbers and ends its self-check with
// "self-check: 7 failures" (exit code 1). The self-check uses the sum program of F1-23, whose
// real numbers are printed in F1-27's table (62 / 20 cycles and stalls without forwarding,
// 47 / 5 with). When all four TODOs are right it reports 0 failures and the exam program's
// diagrams and totals are the ones you compare with your hand trace.
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "../F1-23/u16.h"

struct Row
{
    std::string text;
    int ifc = 0, id = 0, ex = 0, mem = 0, wb = 0;  // cycle of entry into each stage
    bool flushed = false;                           // a fetch on the wrong path after a taken branch
    int lastCycle = 0;                              // flushed rows: last cycle drawn
    bool loadUse = false;                           // marked when TODO 3 says so
};

struct Totals
{
    int instructions = 0, cycles = 0, stalls = 0, branch = 0;
};

// ---------------------------------------------------------------------------------------
// TODO 1 (7 points). Return the registers an instruction READS, as register numbers, with r0 left
// out (r0 is never written, so it never causes a hazard). Use the ISA's rules (F1-24, F1-25):
// port A always reads rs for R- and I-type instructions; port B reads rt for R-type
// instructions, and reads rd when the control signal ReadRdAsB is set (sw, beq, bne, out).
std::vector<unsigned> sourcesOf(const u16::Fields& f)
{
    (void)f;
    return {};  // TODO 1: list the registers this instruction reads (no r0)
}

// TODO 2 (7 points). The earliest cycle in which a reader may enter EX, given the cycles in
// which its newest earlier writer entered EX, MEM and WB (F1-27 Layer 2):
//   without forwarding: the reader may read the register in ID during the writer's WB cycle
//                       (write in the first half, read in the second), so EX is WB + 1;
//   with forwarding:    an ALU result exists after the writer's EX, so EX is writer EX + 1;
//                       a load's value exists only after MEM, so EX is writer MEM + 1.
int earliestEx(int writerEx, int writerMem, int writerWb, bool writerIsLoad, bool forwarding)
{
    (void)writerEx; (void)writerMem; (void)writerWb; (void)writerIsLoad; (void)forwarding;
    return 0;  // TODO 2: the earliest EX cycle of the reader
}

// TODO 3 (7 points). The load-use hazard: true when `producer` is a load that writes a
// register (not r0) which `consumer`, the very next executed instruction, reads.
bool loadUse(const u16::Step& producer, const u16::Step& consumer)
{
    (void)producer; (void)consumer;
    return false;  // TODO 3: is this pair a load-use hazard?
}

// TODO 4 (7 points). CPI = cycles / instructions, and the accounting identity of F1-27:
// cycles = instructions + 4 + data-stall cycles + cycles lost to taken branches.
double cpi(const Totals& t)
{
    (void)t;
    return 0.0;  // TODO 4a: cycles per instruction
}

bool identityHolds(const Totals& t)
{
    (void)t;
    return false;  // TODO 4b: cycles == instructions + 4 + stalls + branch cycles?
}
// ----------------------------------------------------------------------------------------

// Given: the scheduler. It runs the program on the U16 machine to get the executed
// instructions (so branch outcomes are real), then assigns each one its stage cycles.
struct Result
{
    std::vector<Row> rows;
    Totals t;
};

Result schedule(const u16::Program& prog, bool forwarding, long maxSteps = 5000)
{
    u16::Machine m(prog);
    std::vector<u16::Step> trace;
    while (!m.halted && static_cast<long>(trace.size()) < maxSteps) {
        trace.push_back(m.step());
    }
    Result r;
    std::vector<Row> done;
    int nextFetch = 1;
    for (std::size_t i = 0; i < trace.size(); ++i) {
        const u16::Step& s = trace[i];
        Row x;
        x.text = u16::disasm(s.word);
        x.ifc = nextFetch;
        const Row* prev = done.empty() ? nullptr : &done.back();
        x.id = std::max(x.ifc + 1, prev ? prev->ex : 0);        // ID is free when prev left it
        int ex = std::max(x.id + 1, prev ? prev->ex + 1 : 0);   // one instruction per stage
        const int exNoHazard = ex;
        for (const unsigned reg : sourcesOf(s.f)) {
            for (std::size_t k = done.size(); k-- > 0;) {         // newest earlier writer of reg
                const u16::Step& p = trace[k];
                if (p.c.regWrite && p.f.rd == reg) {
                    ex = std::max(ex, earliestEx(done[k].ex, done[k].mem, done[k].wb,
                                                 p.c.memRead, forwarding));
                    break;
                }
            }
        }
        r.t.stalls += ex - exNoHazard;
        x.ex = ex;
        x.mem = ex + 1;
        x.wb = ex + 2;
        x.loadUse = i > 0 && loadUse(trace[i - 1], s);
        nextFetch = std::max(x.ifc + 1, x.id);
        r.rows.push_back(x);
        done.push_back(x);
        if (s.taken) {   // resolved at the end of EX: the fetches behind it are flushed
            int f = nextFetch, id = x.ex;
            for (int k = 1; f <= x.ex; ++k) {
                Row w;
                const std::size_t a = static_cast<std::size_t>(s.pc + k);
                w.text = a < prog.code.size() ? u16::disasm(prog.code[a]) : "(past the end)";
                w.flushed = true;
                w.ifc = f;
                w.id = std::max(f + 1, id);
                w.lastCycle = x.ex;
                r.rows.push_back(w);
                f = std::max(f + 1, w.id);
                id = w.id + 1;
            }
            r.t.branch += x.ex + 2 - std::max(nextFetch + 1, x.ex);
            nextFetch = x.ex + 1;
        }
    }
    r.t.instructions = static_cast<int>(done.size());
    r.t.cycles = done.empty() ? 0 : done.back().wb;
    return r;
}

// Given: the diagram, in the format of the course's pipeline model (capitals = entry into a
// stage, lower case = still there, xx = flushed fetch), plus a load-use marker.
void draw(const Result& r, std::size_t maxRows)
{
    const std::size_t n = std::min(maxRows, r.rows.size());
    int last = 0;
    for (std::size_t i = 0; i < n; ++i) {
        last = std::max(last, r.rows[i].flushed ? r.rows[i].lastCycle : r.rows[i].wb);
    }
    std::printf("%-20s", "cycle:");
    for (int c = 1; c <= last; ++c) {
        std::printf("%3d", c);
    }
    std::printf("\n");
    for (std::size_t i = 0; i < n; ++i) {
        const Row& x = r.rows[i];
        std::printf("%-20s", (x.flushed ? "  (flushed) " + x.text : x.text).c_str());
        for (int c = 1; c <= last; ++c) {
            const char* cell = "  .";
            if (x.flushed) {
                cell = (c >= x.ifc && c <= x.lastCycle) ? " xx" : "  .";
            } else if (c == x.ifc) { cell = " IF"; }
            else if (c > x.ifc && c < x.id) { cell = " if"; }
            else if (c == x.id) { cell = " ID"; }
            else if (c > x.id && c < x.ex) { cell = " id"; }
            else if (c == x.ex) { cell = " EX"; }
            else if (c == x.mem) { cell = " ME"; }
            else if (c == x.wb) { cell = " WB"; }
            std::printf("%s", cell);
        }
        std::printf("%s\n", x.loadUse ? "  <- load-use" : "");
    }
}

void summary(const Result& r)
{
    int lu = 0;
    for (const Row& x : r.rows) {
        lu += x.loadUse ? 1 : 0;
    }
    std::printf("instructions %d, cycles %d, CPI %.2f, data-stall cycles %d, "
                "cycles lost to taken branches %d, load-use pairs %d, identity %s\n",
                r.t.instructions, r.t.cycles, cpi(r.t), r.t.stalls, r.t.branch, lu,
                identityHolds(r.t) ? "holds" : "BROKEN");
}

// Given: the self-check on the sum program of F1-23 (numbers printed in F1-27's table).
const char* kSum =
    ".data 7 3 9 4 6\n"
    "addi r1, r0, 0\naddi r2, r0, 5\naddi r3, r0, 0\n"
    "loop: lw r4, 0(r1)\nadd r3, r3, r4\naddi r1, r1, 1\naddi r2, r2, -1\nbne r2, r0, loop\n"
    "out r3\nhalt\n";

int selfCheck()
{
    std::istringstream in(kSum);
    const u16::Program p = u16::assemble(in);
    int failures = 0;
    auto expect = [&failures](const char* what, long got, long want) {
        const bool ok = got == want;
        std::printf("  %-44s %6ld  (expected %ld) %s\n", what, got, want, ok ? "ok" : "FAIL");
        failures += ok ? 0 : 1;
    };
    const Result slow = schedule(p, false), fast = schedule(p, true);
    int lu = 0;
    for (const Row& x : fast.rows) {
        lu += x.loadUse ? 1 : 0;
    }
    std::printf("self-check on the sum program of F1-23 (F1-27's table):\n");
    expect("cycles without forwarding", slow.t.cycles, 62);
    expect("data-stall cycles without forwarding", slow.t.stalls, 20);
    expect("cycles with forwarding", fast.t.cycles, 47);
    expect("data-stall cycles with forwarding", fast.t.stalls, 5);
    expect("load-use pairs (one per iteration)", lu, 5);
    expect("identity holds in both modes (1 = yes)", identityHolds(slow.t) && identityHolds(fast.t), 1);
    expect("CPI with forwarding x 100", static_cast<long>(cpi(fast.t) * 100 + 0.5), 157);
    std::printf("self-check: %d failures\n\n", failures);
    return failures;
}

int main(int argc, char** argv)
{
    const std::size_t rows = argc > 1 ? static_cast<std::size_t>(std::atoi(argv[1])) : 16;
    const int failures = selfCheck();
    const u16::Program p = u16::assemble(std::cin);
    if (!p.errors.empty()) {
        std::cout << p.errors.front() << '\n';
        return 1;
    }
    std::cout << "== exam program, without forwarding ==\n";
    const Result slow = schedule(p, false);
    draw(slow, rows);
    summary(slow);
    std::cout << "== exam program, with forwarding ==\n";
    const Result fast = schedule(p, true);
    draw(fast, rows);
    summary(fast);
    return failures ? 1 : 0;
}
