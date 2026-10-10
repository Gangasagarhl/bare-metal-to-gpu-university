// pipe.h - a timing model of the classic five-stage pipeline for U16 programs (F1-26).
// Stages: IF (fetch), ID (decode + register read), EX (ALU), MEM (data memory), WB (write
// back). The program is first executed by the U16 machine (u16.h); the model then works out,
// for every executed instruction, the cycle in which it enters each stage, using the rules
// written next to each line below. It is a model of timing, not a gate-level circuit.
#pragma once
#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>
#include "../F1-23/u16.h"

namespace pipe
{

struct Config
{
    bool dataHazards;    // false = pretend no instruction ever waits for another's result
    bool forwarding;     // true = results are passed straight from EX/MEM to a later EX
    bool branchPenalty;  // true = taken branches are found in EX; wrong fetches are flushed
};

struct Row
{
    std::string text;    // the instruction, as disassembled
    int ifc = 0, id = 0, ex = 0, mem = 0, wb = 0;  // cycle of entry into each stage
    bool flushed = false;                           // fetched on the wrong path, then cancelled
    int lastCycle = 0;                              // flushed rows: last cycle drawn
};

struct Result
{
    std::vector<Row> rows;  // executed instructions and flushed wrong-path fetches, in order
    int instructions = 0, cycles = 0, dataStalls = 0, branchLost = 0;
};

// Registers an instruction reads (r0 never causes a hazard: it is always 0).
inline std::vector<unsigned> sources(const u16::Fields& f)
{
    const u16::Control c = u16::control(f.op);
    std::vector<unsigned> s;
    if (u16::isRType(f.op) || u16::isIType(f.op)) {
        s.push_back(f.rs);
    }
    if (u16::isRType(f.op)) {
        s.push_back(f.rt);
    }
    if (c.readRdAsB) {
        s.push_back(f.rd);
    }
    s.erase(std::remove(s.begin(), s.end(), 0u), s.end());
    return s;
}

inline Result schedule(const u16::Program& prog, const Config& cfg, long maxSteps = 5000)
{
    u16::Machine m(prog);
    std::vector<u16::Step> trace;
    while (!m.halted && static_cast<long>(trace.size()) < maxSteps) {
        trace.push_back(m.step());
    }
    Result r;
    std::vector<Row> done;      // executed instructions only (for hazard look-ups)
    int nextFetch = 1;          // earliest cycle the next instruction may enter IF
    for (const u16::Step& s : trace) {
        Row x;
        x.text = u16::disasm(s.word);
        x.ifc = nextFetch;
        const Row* prev = done.empty() ? nullptr : &done.back();
        x.id = std::max(x.ifc + 1, prev ? prev->ex : 0);        // ID is free when prev left it
        int ex = std::max(x.id + 1, prev ? prev->ex + 1 : 0);   // one instruction per stage
        const int exNoHazard = ex;
        if (cfg.dataHazards) {
            for (const unsigned reg : sources(s.f)) {
                for (std::size_t k = done.size(); k-- > 0;) {     // newest earlier writer of reg
                    const u16::Step& p = trace[k];
                    if (!(p.c.regWrite && p.f.rd == reg)) {
                        continue;
                    }
                    const Row& w = done[k];
                    if (!cfg.forwarding) {
                        ex = std::max(ex, w.wb + 1);   // read in ID during WB's cycle or later
                    } else if (p.c.memRead) {
                        ex = std::max(ex, w.mem + 1);  // a load's value exists after MEM
                    } else {
                        ex = std::max(ex, w.ex + 1);   // an ALU value exists after EX
                    }
                    break;
                }
            }
        }
        r.dataStalls += ex - exNoHazard;
        x.ex = ex;
        x.mem = x.ex + 1;
        x.wb = x.mem + 1;
        nextFetch = std::max(x.ifc + 1, x.id);  // the next instruction enters IF as x leaves it
        r.rows.push_back(x);
        done.push_back(x);
        if (cfg.branchPenalty && s.taken) {
            // Fetch went on with pc+1, pc+2...; the branch is resolved at the end of EX, so
            // those fetches are cancelled and the target is fetched in the next cycle.
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
            // without the flush the next instruction would enter ID at max(nextFetch+1, x.ex);
            // now it enters ID at x.ex + 2
            r.branchLost += x.ex + 2 - std::max(nextFetch + 1, x.ex);
            nextFetch = x.ex + 1;
        }
    }
    r.instructions = static_cast<int>(done.size());
    r.cycles = done.empty() ? 0 : done.back().wb;
    return r;
}

// Draw rows [0, maxRows) as a pipeline diagram. A stage name in capitals is the cycle an
// instruction enters that stage; lower case means it is still there (stalled); "xx" marks
// cycles of a fetch that was flushed.
inline void draw(const Result& r, std::size_t maxRows)
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
        std::printf("\n");
    }
}

inline void summary(const Result& r)
{
    std::printf("instructions %d, cycles %d, CPI %.2f, data-stall cycles %d, "
                "cycles lost to taken branches %d\n",
                r.instructions, r.cycles,
                r.instructions ? static_cast<double>(r.cycles) / r.instructions : 0.0,
                r.dataStalls, r.branchLost);
}

}  // namespace pipe
