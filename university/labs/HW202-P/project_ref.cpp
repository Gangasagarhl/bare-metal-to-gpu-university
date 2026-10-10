// project_ref.cpp - HW202 course project, REFERENCE SOLUTION (Lab Engineer, guide 11.5).
// "A working simulated CPU running a program you wrote in its machine language."
// The CPU is the U16 machine of F1-23 extended with one instruction, as the F1-24 lab asks:
//   sll rd, rs, rt   (opcode 12, R-type): rd = rs shifted left by the low 4 bits of rt.
// The programs are written as hex machine words (comments are for people). The program
// checks every hand-encoded word against the ISA's own encoders, runs the Fibonacci program
// (F1-24 mini-project), the 5 x 8 shift program (F1-24 lab step 3) and the regression test
// (the sum program of F1-23 must still output 29), and prints the control row of the new
// opcode in the layout of F1-25's table.
#include <cstdio>
#include <sstream>
#include <string>
#include <vector>
#include "../F1-23/u16.h"

constexpr unsigned SLL = 12;

// The extended CPU: the U16 machine for opcodes 0-11, plus sll handled here.
struct CpuX
{
    u16::Machine m;
    long steps = 0;
    explicit CpuX(const u16::Program& p) : m(p) {}
    void run(long limit = 100000)
    {
        while (!m.halted && steps < limit) {
            const int pc = m.pc;
            if (pc >= 0 && pc < static_cast<int>(m.code.size())) {
                const u16::Fields f = u16::decode(m.code[static_cast<std::size_t>(pc)]);
                if (f.op == SLL) {
                    const std::uint16_t v = static_cast<std::uint16_t>(m.reg[f.rs] << (m.reg[f.rt] & 0xF));
                    if (f.rd != 0) {               // r0 stays 0, as for every other instruction
                        m.reg[f.rd] = v;
                    }
                    m.pc = pc + 1;
                    ++steps;
                    continue;
                }
            }
            m.step();
            ++steps;
        }
    }
};

std::string disasmX(std::uint16_t w)
{
    const u16::Fields f = u16::decode(w);
    if (f.op == SLL) {
        return "sll r" + std::to_string(f.rd) + ", r" + std::to_string(f.rs) + ", r" + std::to_string(f.rt);
    }
    return u16::disasm(w);
}

struct Word
{
    std::uint16_t hand;      // the word as encoded by hand
    std::uint16_t expected;  // the same instruction through the ISA's encoder (the check)
    const char* comment;
};

int runProgram(const char* title, const std::vector<Word>& words, const std::vector<std::uint16_t>& data,
               const std::vector<std::uint16_t>& wantOut)
{
    int failures = 0;
    std::printf("== %s ==\n", title);
    u16::Program p;
    p.data = data;
    for (std::size_t i = 0; i < words.size(); ++i) {
        const bool ok = words[i].hand == words[i].expected;
        std::printf("  %2zu  0x%04X  %-20s %s%s\n", i, words[i].hand, disasmX(words[i].hand).c_str(),
                    words[i].comment, ok ? "" : "   ENCODING MISMATCH");
        failures += ok ? 0 : 1;
        p.code.push_back(words[i].hand);
    }
    CpuX cpu(p);
    cpu.run();
    std::printf("  output:");
    for (const std::uint16_t v : cpu.m.output) {
        std::printf(" %d", v);
    }
    std::printf("\n  halted: %s, instructions executed: %ld\n", cpu.m.halted ? "yes" : "no", cpu.steps);
    const bool outOk = cpu.m.output == wantOut && cpu.m.halted && cpu.m.fault.empty();
    std::printf("  check: %s\n", outOk ? "output as expected" : "WRONG OUTPUT");
    failures += outOk ? 0 : 1;
    return failures;
}

int main()
{
    using u16::encodeI;
    using u16::encodeR;
    int failures = 0;
    // Fibonacci: output the first eight numbers 0 1 1 2 3 5 8 13 with a loop (F1-24 mini-project).
    failures += runProgram("fibonacci.hex (first eight Fibonacci numbers)", {
        {0x6200, encodeI(u16::ADDI, 1, 0, 0),  "addi r1, r0, 0     a = 0"},
        {0x6401, encodeI(u16::ADDI, 2, 0, 1),  "addi r2, r0, 1     b = 1"},
        {0x6608, encodeI(u16::ADDI, 3, 0, 8),  "addi r3, r0, 8     count = 8"},
        {0xB200, encodeR(u16::OUT, 1, 0, 0),   "loop: out r1       print a"},
        {0x1850, encodeR(u16::ADD, 4, 1, 2),   "add r4, r1, r2     c = a + b"},
        {0x1280, encodeR(u16::ADD, 1, 2, 0),   "add r1, r2, r0     a = b"},
        {0x1500, encodeR(u16::ADD, 2, 4, 0),   "add r2, r4, r0     b = c"},
        {0x66FF, encodeI(u16::ADDI, 3, 3, -1), "addi r3, r3, -1    count = count - 1"},
        {0xA63A, encodeI(u16::BNE, 3, 0, -6),  "bne r3, r0, loop   offset 3 - (8 + 1) = -6"},
        {0x0000, 0,                            "halt"}},
        {}, {0, 1, 1, 2, 3, 5, 8, 13});
    // 5 x 8 with the new shift instruction (F1-24 lab, step 3): 5 << 3 = 40.
    failures += runProgram("shift.hex (5 x 8 as 5 << 3 with the new sll)", {
        {0x6205, encodeI(u16::ADDI, 1, 0, 5), "addi r1, r0, 5"},
        {0x6403, encodeI(u16::ADDI, 2, 0, 3), "addi r2, r0, 3"},
        {0xC650, encodeR(SLL, 3, 1, 2),       "sll r3, r1, r2     r3 = r1 << (r2 & 15)"},
        {0xB600, encodeR(u16::OUT, 3, 0, 0),  "out r3"},
        {0x0000, 0,                           "halt"}},
        {}, {40});
    // Regression test: the sum program of F1-23 on the extended CPU must still output 29.
    std::istringstream sum(".data 7 3 9 4 6\naddi r1, r0, 0\naddi r2, r0, 5\naddi r3, r0, 0\n"
                           "loop: lw r4, 0(r1)\nadd r3, r3, r4\naddi r1, r1, 1\naddi r2, r2, -1\n"
                           "bne r2, r0, loop\nout r3\nhalt\n");
    const u16::Program sp = u16::assemble(sum);
    CpuX reg(sp);
    reg.run();
    const bool regOk = reg.m.output.size() == 1 && reg.m.output[0] == 29 && reg.steps == 30;
    std::printf("== regression: the sum program of F1-23 on the extended CPU ==\n  output: %d, instructions executed: %ld, check: %s\n",
                reg.m.output.empty() ? -1 : reg.m.output[0], reg.steps, regOk ? "still 29 in 30 instructions" : "REGRESSION");
    failures += regOk ? 0 : 1;
    // The control row the new opcode needs (F1-25 layout): RegWrite 1, everything else 0, ALU = shift.
    std::printf("== control row for opcode 12 (F1-25 layout) ==\n"
                "op  name  RegWr AluImm RdAsB MemRd MemWr MemToReg BrEq BrNe Out Halt ALU\n"
                "12  sll       1      0     0     0     0        0    0    0   0    0 sll\n");
    std::printf("project reference: %d failures\n", failures);
    return failures ? 1 : 0;
}
