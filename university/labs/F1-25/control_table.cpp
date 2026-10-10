// F1-25 Listing 1: print the U16 control unit as a truth table (one row per opcode).
// The table comes from u16::control(), the function the simulators use, so the table
// and the simulators can never disagree.
#include <cstdio>
#include "../F1-23/u16.h"

int main()
{
    std::printf("op  name  RegWr AluImm RdAsB MemRd MemWr MemToReg BrEq BrNe Out Halt ALU\n");
    for (unsigned op = 0; op < 16; ++op) {
        const u16::Control c = u16::control(op);
        const char* name = op < u16::kNames.size() ? u16::kNames[op] : "-";
        std::printf("%2u  %-5s %5d %6d %5d %5d %5d %8d %4d %4d %3d %4d %s\n", op, name,
                    c.regWrite, c.aluSrcImm, c.readRdAsB, c.memRead, c.memWrite, c.memToReg,
                    c.branchEq, c.branchNe, c.out, c.halt,
                    u16::kAluNames[static_cast<std::size_t>(c.alu)]);
    }
    return 0;
}
