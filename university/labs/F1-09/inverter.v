// A CMOS inverter described transistor by transistor (Verilog switch-level primitives).
// pmos/nmos instance ports are (output side, input side, gate).
module inverter(input wire a, output wire y);
    supply1 vdd;           // the supply rail: logic 1
    supply0 gnd;           // ground: logic 0

    pmos p1(y, vdd, a);    // pull-up: conducts when a is 0
    nmos n1(y, gnd, a);    // pull-down: conducts when a is 1
endmodule
