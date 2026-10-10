// A simplified model of one SRAM cell: two cross-coupled inverters hold a bit on the
// nodes q and qn. Access switches connect the nodes to the bit lines while the word
// line is 1. To write, strong bit-line drivers overpower the cell's weak inverters.
// (A teaching model: a real cell is a transistor circuit; see the chapter text.)
`timescale 1ns/1ns
module sram_cell (
    input  wire wl,      // word line: 1 connects the cell to the bit lines
    inout  wire bl,      // bit line
    inout  wire bln      // the complementary bit line
);
    wire q;
    wire qn;
    // the storage loop: weak inverters, so a strong driver can flip them
    not (weak0, weak1) #1 inv1 (q, qn);
    not (weak0, weak1) #1 inv2 (qn, q);
    // the access switches (pass both ways while wl = 1)
    tranif1 access1 (q, bl, wl);
    tranif1 access2 (qn, bln, wl);
endmodule
