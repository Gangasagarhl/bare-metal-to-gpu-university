// NOT, NAND and NOR built transistor by transistor (Verilog switch-level primitives).
// pmos/nmos instance ports are (output side, input side, gate).

module cmos_not(input wire a, output wire y);
    supply1 vdd;
    supply0 gnd;
    pmos p1(y, vdd, a);        // pull-up
    nmos n1(y, gnd, a);        // pull-down
endmodule

module cmos_nand2(input wire a, input wire b, output wire y);
    supply1 vdd;
    supply0 gnd;
    wire mid;                  // node between the two series nMOS
    pmos p1(y, vdd, a);        // pull-up: two pMOS in parallel
    pmos p2(y, vdd, b);
    nmos n1(y, mid, a);        // pull-down: two nMOS in series
    nmos n2(mid, gnd, b);
endmodule

module cmos_nor2(input wire a, input wire b, output wire y);
    supply1 vdd;
    supply0 gnd;
    wire mid;                  // node between the two series pMOS
    pmos p1(mid, vdd, a);      // pull-up: two pMOS in series
    pmos p2(y, mid, b);
    nmos n1(y, gnd, a);        // pull-down: two nMOS in parallel
    nmos n2(y, gnd, b);
endmodule
