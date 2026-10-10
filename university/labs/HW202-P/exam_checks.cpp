// exam_checks.cpp - recomputes every number quoted in the HW202 answer keys (midterm, final,
// forensic and design questions) from the course's own code: the U16 ISA (u16.h), the
// pipeline timing model (pipe.h), the predictor rules of F1-28, the issue bounds of F1-29,
// Amdahl's law and the interleaving replay of F1-30. Nothing here is typed in by hand.
#include <cmath>
#include <fstream>
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include "../F1-23/u16.h"
#include "../F1-26/pipe.h"

static void word(const char* label, std::uint16_t w)
{
    const u16::Fields f = u16::decode(w);
    std::printf("  %-28s 0x%04X = op %2u rd %u rs %u rt %u imm %3d  -> %s\n", label, w, f.op, f.rd,
                f.rs, f.rt, f.imm, u16::disasm(w).c_str());
}

static pipe::Result model(const std::string& src, bool hazards, bool forwarding, bool branch)
{
    std::istringstream in(src);
    const u16::Program p = u16::assemble(in);
    if (!p.errors.empty()) {
        std::printf("  ASSEMBLY ERROR: %s\n", p.errors.front().c_str());
    }
    return pipe::schedule(p, pipe::Config{hazards, forwarding, branch});
}

static void totals(const char* label, const pipe::Result& r)
{
    std::printf("  %-40s instructions %d, cycles %d, CPI %.2f, data stalls %d, branch cycles %d, identity %s\n",
                label, r.instructions, r.cycles, static_cast<double>(r.cycles) / r.instructions,
                r.dataStalls, r.branchLost,
                r.cycles == r.instructions + 4 + r.dataStalls + r.branchLost ? "holds" : "BROKEN");
}

static void rows(const pipe::Result& r)
{
    for (const pipe::Row& x : r.rows) {
        if (!x.flushed) {
            std::printf("    %-20s IF %2d ID %2d EX %2d MEM %2d WB %2d\n", x.text.c_str(), x.ifc, x.id, x.ex, x.mem, x.wb);
        }
    }
}

static u16::Machine runText(const std::string& src)
{
    std::istringstream in(src);
    u16::Machine m(u16::assemble(in));
    for (int i = 0; i < 100000 && !m.halted; ++i) {
        m.step();
    }
    return m;
}

// --- predictors of F1-28 Listing 1 (kinds: 0 always N, 1 always T, 2 one-bit, 3 two-bit) ---
static int mispredictions(int kind, int state, const std::vector<bool>& outcomes, std::string* trace)
{
    int miss = 0;
    for (const bool taken : outcomes) {
        bool guess = false;
        if (kind == 1) guess = true;
        if (kind == 2) guess = state != 0;
        if (kind == 3) guess = state >= 2;
        const bool wrong = guess != taken;
        miss += wrong ? 1 : 0;
        if (trace) {
            *trace += wrong ? 'X' : '.';
        }
        if (kind == 2) state = taken ? 1 : 0;
        if (kind == 3) state = taken ? std::min(3, state + 1) : std::max(0, state - 1);
    }
    return miss;
}

static int interleave(const std::string& order)
{
    int memory = 0, reg[3] = {0, 0, 0}, step[3] = {0, 0, 0};
    for (const char ch : order) {
        const int c = ch - 'A';
        if (step[c] == 0) reg[c] = memory;
        else if (step[c] == 1) reg[c] += 1;
        else memory = reg[c];
        step[c] = (step[c] + 1) % 3;
    }
    return memory;
}

int main()
{
    std::printf("HW202 exam checks (every number in the answer keys comes from here or from a named lab run)\n\n");

    std::printf("[M2] encodings and a decoding\n");
    word("sub r4, r2, r5", u16::encodeR(u16::SUB, 4, 2, 5));
    word("addi r3, r1, -7", u16::encodeI(u16::ADDI, 3, 1, -7));
    word("decode 0x9A3E", 0x9A3E);
    std::printf("[M3] bne r3, r0, loop at address 11, loop at 4\n");
    std::printf("  offset = 4 - (11 + 1) = %d; 6-bit field = 0x%02X\n", 4 - 12, (4 - 12) & 0x3F);
    word("bne r3, r0, -8", u16::encodeI(u16::BNE, 3, 0, 4 - 12));
    std::printf("  taken target check: 11 + 1 + (%d) = %d\n", 4 - 12, 11 + 1 + (4 - 12));
    std::printf("[M5] a word of the multiply program\n");
    word("decode 0x16C8", 0x16C8);
    std::printf("[M6] sw r5, 3(r2) with r2 = 20, r5 = 9 (machine trace)\n");
    {
        std::istringstream in("addi r2, r0, 20\naddi r5, r0, 9\nsw r5, 3(r2)\nhalt\n");
        u16::Machine m(u16::assemble(in));
        m.step(); m.step();
        const u16::Step s = m.step();
        std::printf("  A(rs=r%u) = %u, B(port B reads r%u) = %u, ALU input B = imm %d, ALU out = %u, memory[%u] = %u, register written: %s\n",
                    s.f.rs, s.a, s.c.readRdAsB ? s.f.rd : s.f.rt, s.b, s.f.imm, s.aluOut, s.aluOut, m.mem[s.aluOut],
                    s.wrote ? "yes" : "no");
    }
    std::printf("[M7] beq r1, r4, -3 at address 9 with r1 = r4 = 12\n");
    {
        u16::Program p;
        p.code.assign(10, 0);
        p.code[9] = u16::encodeI(u16::BEQ, 1, 4, -3);
        u16::Machine m(p);
        m.reg[1] = 12; m.reg[4] = 12; m.pc = 9;
        const u16::Step s = m.step();
        std::printf("  word 0x%04X; A(rs=r%u) = %u, B(rd=r%u) = %u, ALU (sub) = %u, taken = %s, next PC = %d\n",
                    s.word, s.f.rs, s.a, s.f.rd, s.b, s.aluOut, s.taken ? "yes" : "no", s.nextPc);
    }
    std::printf("[M8] single-cycle clock (exercise values): lw 900, add 650, beq 500 du; 40 lw, 40 add, 20 beq\n");
    {
        const long fixed = 100L * 900, own = 40L * 900 + 40L * 650 + 20L * 500;
        std::printf("  fixed period 900 du: %ld du; each instruction at its own time: %ld du; ratio %.2f; waste %ld du\n",
                    fixed, own, static_cast<double>(fixed) / own, fixed - own);
    }
    std::printf("[M9] port B for addi r2, r2, -1 (word 0x64BF): rt field = bits 5-3\n");
    word("0x64BF", 0x64BF);
    std::printf("[M11] lw with AluSrcImm stuck at 0: the ALU adds register rt (bits 5-3 of the immediate)\n");
    word("lw r4, 0(r1)", u16::encodeI(u16::LW, 4, 1, 0));
    word("lw r5, 4(r1)", u16::encodeI(u16::LW, 5, 1, 4));
    word("lw r4, 8(r1)", u16::encodeI(u16::LW, 4, 1, 8));
    std::printf("[M13] ideal pipeline: 12 instructions -> %d cycles, CPI %.2f; multiply program of F1-24:\n", 12 + 4, 16.0 / 12);
    {
        const pipe::Result r = model("addi r1, r0, 6\naddi r2, r0, 7\naddi r3, r0, 0\nloop: add r3, r3, r1\naddi r2, r2, -1\nbne r2, r0, loop\nout r3\nhalt\n", false, false, false);
        totals("multiply, ideal", r);
        const pipe::Result f = model("addi r1, r0, 6\naddi r2, r0, 7\naddi r3, r0, 0\nloop: add r3, r3, r1\naddi r2, r2, -1\nbne r2, r0, loop\nout r3\nhalt\n", true, true, true);
        totals("multiply, forwarding (F1-27 check 8)", f);
    }
    std::printf("[M14/M15] stage delays IF 180, ID 120, EX 220, MEM 300, WB 100 du, register overhead 20 (exercise values)\n");
    {
        const int d[5] = {180, 120, 220, 300, 100}, r = 20;
        int sum = 0, mx = 0;
        for (int x : d) { sum += x; mx = std::max(mx, x); }
        const int single = sum, piped = mx + r;
        std::printf("  single-cycle period %d du; pipelined period %d du\n", single, piped);
        for (const long n : {1L, 5L, 200L, 100000L}) {
            const long ts = n * single, tp = (5 + n - 1) * piped;
            std::printf("  n = %6ld: single %9ld du, pipelined %9ld du (%ld cycles), speed-up %.3f\n", n, ts, tp, 5 + n - 1, static_cast<double>(ts) / tp);
        }
        std::printf("  limit %.3f\n", static_cast<double>(single) / piped);
        const int piped6 = std::max({180, 120, 220, 150, 150, 100}) + r;
        std::printf("  MEM split into 150 + 150 (six stages): period %d du; n = 200: %ld du (%d cycles), speed-up %.3f; limit %.3f\n",
                    piped6, (6 + 200 - 1) * static_cast<long>(piped6), 6 + 200 - 1, 200.0 * single / ((6 + 200 - 1) * piped6),
                    static_cast<double>(single) / piped6);
    }

    std::printf("\n[F2] decode 0x5B30; encode sw r2, -1(r1)\n");
    word("decode 0x5B30", 0x5B30);
    word("sw r2, -1(r1)", u16::encodeI(u16::SW, 2, 1, -1));
    std::printf("[F3] lw r6, 2(r3) with r3 = 5 and memory[7] = 42\n");
    {
        std::istringstream in(".data 0 0 0 0 0 0 0 42\naddi r3, r0, 5\nlw r6, 2(r3)\nhalt\n");
        u16::Machine m(u16::assemble(in));
        m.step();
        const u16::Step s = m.step();
        std::printf("  A = %u, ALU input B = imm %d, ALU out (address) = %u, memory read = %u, written r%u = %u, MemToReg = %d\n",
                    s.a, s.f.imm, s.aluOut, s.value, s.f.rd, m.reg[6], s.c.memToReg);
    }
    std::printf("[F4] sw with ReadRdAsB stuck at 0: port B reads rt = bits 5-3 of the immediate\n");
    word("sw r3, 0(r1)", u16::encodeI(u16::SW, 3, 1, 0));
    word("sw r3, 9(r1)", u16::encodeI(u16::SW, 3, 1, 9));
    std::printf("[F5] ideal pipeline, 55 instructions: %d cycles, CPI %.3f, fill share %d/%d = %.1f %%\n", 59, 59.0 / 55, 4, 59, 400.0 / 59);
    std::printf("[F6] lw r1,0(r2); addi r3,r1,4; sw r3,1(r2); add r4,r3,r1 (data 5)\n");
    {
        const std::string s = ".data 5\nlw r1, 0(r2)\naddi r3, r1, 4\nsw r3, 1(r2)\nadd r4, r3, r1\nhalt\n";
        const pipe::Result a = model(s, true, false, true), b = model(s, true, true, true);
        std::printf("  without forwarding:\n"); rows(a); totals("F6 no forwarding", a);
        std::printf("  with forwarding:\n"); rows(b); totals("F6 forwarding", b);
    }
    std::printf("[F7] the copy loop (5 numbers from 0-4 to 10-14), with forwarding\n");
    {
        const std::string orig = ".data 7 3 9 4 6\naddi r1, r0, 0\naddi r2, r0, 5\nloop: lw r4, 0(r1)\nsw r4, 10(r1)\naddi r1, r1, 1\naddi r2, r2, -1\nbne r2, r0, loop\nhalt\n";
        const std::string sched = ".data 7 3 9 4 6\naddi r1, r0, 0\naddi r2, r0, 5\nloop: lw r4, 0(r1)\naddi r1, r1, 1\nsw r4, 9(r1)\naddi r2, r2, -1\nbne r2, r0, loop\nhalt\n";
        totals("copy, as written, forwarding", model(orig, true, true, true));
        totals("copy, as written, no forwarding", model(orig, true, false, true));
        totals("copy, scheduled, forwarding", model(sched, true, true, true));
        for (const auto* v : {&orig, &sched}) {
            const u16::Machine m = runText(*v);
            std::printf("  memory[10..14] after the run: %u %u %u %u %u (%s)\n", m.mem[10], m.mem[11], m.mem[12], m.mem[13], m.mem[14],
                        v == &orig ? "as written" : "scheduled");
        }
    }
    std::printf("[F8] 2-bit counter from state 3 on N N T T T N T T\n");
    {
        std::string t;
        const int miss = mispredictions(3, 3, {false, false, true, true, true, false, true, true}, &t);
        std::printf("  mispredictions %d of 8, trace %s, cycles lost at 2 each: %d\n", miss, t.c_str(), 2 * miss);
    }
    std::printf("[F9] loop branch taken 7 times then not taken, 20 loop executions (160 branches)\n");
    {
        std::vector<bool> o;
        for (int e = 0; e < 20; ++e) {
            for (int i = 0; i < 7; ++i) o.push_back(true);
            o.push_back(false);
        }
        const char* names[4] = {"always not taken", "always taken", "1-bit (starts N)", "2-bit (starts 0)"};
        for (int k = 0; k < 4; ++k) {
            std::string t;
            const int miss = mispredictions(k, 0, o, &t);
            std::printf("  %-18s mispredictions %3d, cycles lost %3d, first 16: %s\n", names[k], miss, 2 * miss, t.substr(0, 16).c_str());
        }
        std::printf("  loop body of 8 instructions: %d instructions; CPI from branches alone = 1 + lost/instructions\n", 20 * 8);
    }
    std::printf("[F11] bounds for the two-sum sequence C of F1-29 (latencies 3): dependence 3 + 4*3 + 3 = %d; issue 17/2 -> %d cycles\n", 3 + 12 + 3, (17 + 1) / 2);
    std::printf("[F13] Amdahl p = 0.8: N = 4 -> %.2f, N = 16 -> %.2f, limit %.1f\n", 1 / (0.2 + 0.8 / 4), 1 / (0.2 + 0.8 / 16), 1 / 0.2);
    std::printf("[F14] interleavings of three cores doing load-add-store: AAABCBCBC -> memory %d; ABCABCABC -> %d; AAABBBCCC -> %d\n",
                interleave("AAABCBCBC"), interleave("ABCABCABC"), interleave("AAABBBCCC"));
    std::printf("[F18] design reference: count the readings equal to 9 among 9 4 9 7, scheduled for forwarding\n");
    {
        const std::string eq = ".data 9 4 9 7\naddi r1, r0, 0\naddi r2, r0, 4\naddi r3, r0, 0\naddi r6, r0, 9\n"
                               "loop: lw r4, 0(r1)\naddi r1, r1, 1\nsub r5, r4, r6\nbne r5, r0, skip\naddi r3, r3, 1\n"
                               "skip: addi r2, r2, -1\nbne r2, r0, loop\nout r3\nhalt\n";
        totals("equal-count, forwarding", model(eq, true, true, true));
        totals("equal-count, no forwarding", model(eq, true, false, true));
        const u16::Machine m = runText(eq);
        std::printf("  output: %u\n", m.output.empty() ? 999 : m.output[0]);
    }
    std::printf("\n[P] the practical's exam program through the course model (must equal pipe_exam.out)\n");
    {
        std::ifstream f("pipe_exam.in");
        std::stringstream ss; ss << f.rdbuf();
        totals("range.s, no forwarding", model(ss.str(), true, false, true));
        totals("range.s, forwarding", model(ss.str(), true, true, true));
        const u16::Machine m = runText(ss.str());
        std::printf("  output: %u, memory[4] = %u\n", m.output.empty() ? 999 : m.output[0], m.mem[4]);
    }
    std::printf("[P] the exam program scheduled by the key (addi r1 between lw r4 and slt; addi r2 between lw r6 and add r7)\n");
    {
        const std::string sched = ".data 14 3 27 9\naddi r1, r0, 0\nlw r6, 0(r1)\naddi r2, r0, 4\nadd r7, r6, r0\n"
                                  "loop: lw r4, 0(r1)\naddi r1, r1, 1\nslt r5, r6, r4\nbeq r5, r0, skip1\nadd r6, r4, r0\n"
                                  "skip1: slt r5, r4, r7\nbeq r5, r0, skip2\nadd r7, r4, r0\nskip2: addi r2, r2, -1\nbne r2, r0, loop\n"
                                  "sub r3, r6, r7\nout r3\nsw r3, 4(r0)\nhalt\n";
        totals("range.s scheduled, forwarding", model(sched, true, true, true));
        const u16::Machine m = runText(sched);
        std::printf("  output: %u, memory[4] = %u\n", m.output.empty() ? 999 : m.output[0], m.mem[4]);
    }
    return 0;
}
