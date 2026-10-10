// u16.h - U16, the university's 16-bit teaching instruction set (HW202, chapter F1-23).
// One header holds the whole ISA: encoding, decoding, control signals, an assembler,
// a disassembler and a machine that executes one instruction per call (single-cycle).
// Later labs (F1-24 to F1-29) include this same file, so every chapter uses one ISA.
#pragma once
#include <array>
#include <cstdint>
#include <istream>
#include <sstream>
#include <string>
#include <vector>

namespace u16
{

// ---- 1. Opcodes: the 4 high bits of every instruction word ----------------------------
enum Op : unsigned { HALT = 0, ADD, SUB, AND, OR, SLT, ADDI, LW, SW, BEQ, BNE, OUT };
const std::array<const char*, 12> kNames = {"halt", "add", "sub", "and", "or", "slt",
                                            "addi", "lw", "sw", "beq", "bne", "out"};

// ---- 2. Instruction formats -------------------------------------------------------------
//  R-type  | op:4 | rd:3 | rs:3 | rt:3 | 000 |      add sub and or slt
//  I-type  | op:4 | rd:3 | rs:3 |   imm:6     |      addi lw sw beq bne  (imm is signed)
//  X-type  | op:4 | rd:3 |  000000000        |      out halt
struct Fields
{
    unsigned op, rd, rs, rt;
    int imm;  // sign-extended from 6 bits: -32 .. 31
};

inline Fields decode(std::uint16_t w)
{
    Fields f{};
    f.op = (w >> 12) & 0xFu;
    f.rd = (w >> 9) & 0x7u;
    f.rs = (w >> 6) & 0x7u;
    f.rt = (w >> 3) & 0x7u;
    f.imm = static_cast<int>(w & 0x3Fu);
    if (f.imm & 0x20) {
        f.imm -= 64;  // bit 5 is the sign bit of the 6-bit two's-complement field
    }
    return f;
}

inline bool isRType(unsigned op) { return op >= ADD && op <= SLT; }
inline bool isIType(unsigned op) { return op >= ADDI && op <= BNE; }

inline std::uint16_t encodeR(unsigned op, unsigned rd, unsigned rs, unsigned rt)
{
    return static_cast<std::uint16_t>((op << 12) | (rd << 9) | (rs << 6) | (rt << 3));
}

inline std::uint16_t encodeI(unsigned op, unsigned rd, unsigned rs, int imm)
{
    const unsigned field = static_cast<unsigned>(imm) & 0x3Fu;  // keep the low 6 bits
    return static_cast<std::uint16_t>((op << 12) | (rd << 9) | (rs << 6) | field);
}

// ---- 3. Control signals: what the control unit (F1-25) sets for each opcode -------------
enum class Alu { Add, Sub, And, Or, Slt };
const std::array<const char*, 5> kAluNames = {"add", "sub", "and", "or", "slt"};

struct Control
{
    bool regWrite;   // write the result into register rd
    bool aluSrcImm;  // ALU input B is the immediate (1) or the second register read (0)
    bool readRdAsB;  // second register read port uses field rd (1) or field rt (0)
    bool memRead;    // load: read data memory
    bool memWrite;   // store: write data memory
    bool memToReg;   // value written to rd comes from memory (1) or from the ALU (0)
    bool branchEq;   // beq: take the branch when the ALU result is zero
    bool branchNe;   // bne: take the branch when the ALU result is not zero
    bool out;        // out: send the second register read to the output port
    bool halt;       // halt: stop the machine
    Alu alu;         // which operation the ALU performs
};

inline Control control(unsigned op)
{
    Control c{false, false, false, false, false, false, false, false, false, false, Alu::Add};
    switch (op) {
    case ADD: c.regWrite = true; c.alu = Alu::Add; break;
    case SUB: c.regWrite = true; c.alu = Alu::Sub; break;
    case AND: c.regWrite = true; c.alu = Alu::And; break;
    case OR:  c.regWrite = true; c.alu = Alu::Or;  break;
    case SLT: c.regWrite = true; c.alu = Alu::Slt; break;
    case ADDI: c.regWrite = true; c.aluSrcImm = true; break;
    case LW: c.regWrite = true; c.aluSrcImm = true; c.memRead = true; c.memToReg = true; break;
    case SW: c.aluSrcImm = true; c.readRdAsB = true; c.memWrite = true; break;
    case BEQ: c.readRdAsB = true; c.branchEq = true; c.alu = Alu::Sub; break;
    case BNE: c.readRdAsB = true; c.branchNe = true; c.alu = Alu::Sub; break;
    case OUT: c.readRdAsB = true; c.out = true; break;
    default: c.halt = true; break;  // HALT and the unused opcodes 12..15 stop the machine
    }
    return c;
}

inline std::uint16_t aluCompute(Alu op, std::uint16_t a, std::uint16_t b)
{
    switch (op) {
    case Alu::Add: return static_cast<std::uint16_t>(a + b);  // wraps modulo 2^16
    case Alu::Sub: return static_cast<std::uint16_t>(a - b);
    case Alu::And: return static_cast<std::uint16_t>(a & b);
    case Alu::Or:  return static_cast<std::uint16_t>(a | b);
    case Alu::Slt: return static_cast<std::int16_t>(a) < static_cast<std::int16_t>(b) ? 1 : 0;
    }
    return 0;
}

// ---- 4. Disassembler: machine word -> text ----------------------------------------------
inline std::string disasm(std::uint16_t w)
{
    const Fields f = decode(w);
    std::ostringstream s;
    if (f.op >= kNames.size()) {
        s << "(unused opcode " << f.op << ")";
        return s.str();
    }
    s << kNames[f.op];
    if (isRType(f.op)) {
        s << " r" << f.rd << ", r" << f.rs << ", r" << f.rt;
    } else if (f.op == LW || f.op == SW) {
        s << " r" << f.rd << ", " << f.imm << "(r" << f.rs << ")";
    } else if (isIType(f.op)) {
        s << " r" << f.rd << ", r" << f.rs << ", " << f.imm;
    } else if (f.op == OUT) {
        s << " r" << f.rd;
    }
    return s.str();
}

// ---- 5. Assembler: text -> machine words --------------------------------------------
// Syntax: one instruction per line; "label:" before it is optional; ';' starts a comment.
// ".data 7 3 9" puts words into data memory from address 0. A line that is a hexadecimal
// word such as "0x1234" is taken as machine code as it stands (write your own encodings).
struct Program
{
    std::vector<std::uint16_t> code;
    std::vector<std::uint16_t> data;
    std::vector<std::string> errors;
};

inline std::string clean(std::string s)
{
    const auto semi = s.find(';');
    if (semi != std::string::npos) {
        s.erase(semi);
    }
    for (char& ch : s) {
        if (ch == ',' || ch == '(' || ch == ')' || ch == '\t') {
            ch = ' ';
        }
    }
    return s;
}

inline int parseReg(const std::string& t, bool& ok)
{
    if (t.size() == 2 && t[0] == 'r' && t[1] >= '0' && t[1] <= '7') {
        return t[1] - '0';
    }
    ok = false;
    return 0;
}

inline Program assemble(std::istream& in)
{
    Program p;
    std::vector<std::string> lines;
    std::vector<std::pair<std::string, int>> labels;
    std::string line;
    int pc = 0;
    // Pass 1: remember where every label is.
    while (std::getline(in, line)) {
        lines.push_back(line);
        std::istringstream ls(clean(line));
        std::string tok;
        if (!(ls >> tok)) {
            continue;
        }
        if (tok.back() == ':') {
            labels.emplace_back(tok.substr(0, tok.size() - 1), pc);
            if (!(ls >> tok)) {
                continue;
            }
        }
        if (tok != ".data") {
            ++pc;
        }
    }
    auto findLabel = [&labels](const std::string& name, bool& ok) {
        for (const auto& [label, addr] : labels) {
            if (label == name) {
                return addr;
            }
        }
        ok = false;
        return 0;
    };
    // Pass 2: encode every instruction.
    for (std::size_t n = 0; n < lines.size(); ++n) {
        std::istringstream ls(clean(lines[n]));
        std::vector<std::string> t;
        for (std::string tok; ls >> tok;) {
            t.push_back(tok);
        }
        if (!t.empty() && t[0].back() == ':') {
            t.erase(t.begin());
        }
        if (t.empty()) {
            continue;
        }
        bool ok = true;
        const int here = static_cast<int>(p.code.size());
        if (t[0] == ".data") {
            for (std::size_t i = 1; i < t.size(); ++i) {
                p.data.push_back(static_cast<std::uint16_t>(std::stoi(t[i], nullptr, 0)));
            }
            continue;
        }
        if (t[0].rfind("0x", 0) == 0) {
            p.code.push_back(static_cast<std::uint16_t>(std::stoul(t[0], nullptr, 16)));
            continue;
        }
        unsigned op = 0;
        while (op < kNames.size() && t[0] != kNames[op]) {
            ++op;
        }
        std::uint16_t word = 0;
        if (op == kNames.size()) {
            ok = false;
        } else if (isRType(op) && t.size() == 4) {
            const int rd = parseReg(t[1], ok), rs = parseReg(t[2], ok), rt = parseReg(t[3], ok);
            word = encodeR(op, rd, rs, rt);
        } else if ((op == LW || op == SW) && t.size() == 4) {   // lw rd, imm(rs)
            const int rd = parseReg(t[1], ok), rs = parseReg(t[3], ok);
            word = encodeI(op, rd, rs, std::stoi(t[2]));
        } else if (op == ADDI && t.size() == 4) {
            const int rd = parseReg(t[1], ok), rs = parseReg(t[2], ok);
            word = encodeI(op, rd, rs, std::stoi(t[3]));
        } else if ((op == BEQ || op == BNE) && t.size() == 4) { // offset = target - (pc + 1)
            const int rd = parseReg(t[1], ok), rs = parseReg(t[2], ok);
            const int target = findLabel(t[3], ok);
            word = encodeI(op, rd, rs, target - (here + 1));
        } else if (op == OUT && t.size() == 2) {
            word = encodeR(op, parseReg(t[1], ok), 0, 0);
        } else if (op == HALT && t.size() == 1) {
            word = 0;
        } else {
            ok = false;
        }
        if (!ok) {
            p.errors.push_back("line " + std::to_string(n + 1) + ": cannot assemble: " + lines[n]);
        }
        p.code.push_back(word);
    }
    return p;
}

// ---- 6. The machine: state plus one step (fetch, decode, execute, memory, write back) ------
struct Step
{
    int pc;                // address of the instruction executed
    std::uint16_t word;    // the instruction word fetched
    Fields f;              // its fields
    Control c;             // the control signals for it
    std::uint16_t a, b;    // register values read (port A = rs, port B = rt or rd)
    std::uint16_t aluOut;  // ALU result
    bool taken;            // branch taken?
    int nextPc;            // address of the next instruction
    bool wrote;            // did it write a register?
    std::uint16_t value;   // value written to rd (or sent to the output port)
};

struct Machine
{
    std::array<std::uint16_t, 8> reg{};
    std::array<std::uint16_t, 256> mem{};
    std::vector<std::uint16_t> code;
    std::vector<std::uint16_t> output;
    int pc = 0;
    bool halted = false;
    std::string fault;

    explicit Machine(const Program& p) : code(p.code)
    {
        for (std::size_t i = 0; i < p.data.size() && i < mem.size(); ++i) {
            mem[i] = p.data[i];
        }
    }

    Step step()
    {
        Step s{};
        s.pc = pc;
        if (pc < 0 || pc >= static_cast<int>(code.size())) {
            fault = "pc " + std::to_string(pc) + " is outside the program";
            halted = true;
            return s;
        }
        s.word = code[static_cast<std::size_t>(pc)];                 // fetch
        s.f = decode(s.word);                                        // decode
        s.c = control(s.f.op);
        s.a = reg[s.f.rs];
        s.b = reg[s.c.readRdAsB ? s.f.rd : s.f.rt];
        const std::uint16_t aluB = s.c.aluSrcImm ? static_cast<std::uint16_t>(s.f.imm) : s.b;
        s.aluOut = aluCompute(s.c.alu, s.a, aluB);                   // execute
        s.value = s.aluOut;
        if (s.c.memRead || s.c.memWrite) {                           // memory
            if (s.aluOut >= mem.size()) {
                fault = "data address " + std::to_string(s.aluOut) + " is outside memory";
                halted = true;
                return s;
            }
            if (s.c.memRead) {
                s.value = mem[s.aluOut];
            } else {
                mem[s.aluOut] = s.b;
            }
        }
        if (s.c.out) {
            output.push_back(s.b);
            s.value = s.b;
        }
        s.taken = (s.c.branchEq && s.aluOut == 0) || (s.c.branchNe && s.aluOut != 0);
        s.nextPc = s.taken ? pc + 1 + s.f.imm : pc + 1;
        if (s.c.regWrite && s.f.rd != 0) {                           // write back
            reg[s.f.rd] = s.value;                                   // r0 always stays 0
            s.wrote = true;
        }
        if (s.c.halt) {
            halted = true;
            s.nextPc = pc;
        }
        pc = s.nextPc;
        return s;
    }
};

}  // namespace u16
