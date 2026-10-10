// Evidence for final question F2: an inverter whose pull-up gate is wired to ground.
// pmos/nmos instance ports are (output side, input side, gate).
module inverter(input wire a, output wire y);
    supply1 vdd;
    supply0 gnd;
    pmos p1(y, vdd, gnd);   // pull-up: gate joined to gnd, so it is always on
    nmos n1(y, gnd, a);     // pull-down: conducts when a is 1
endmodule
