// Forensic evidence: Sam's inverter, exactly as handed in.
module inverter(input wire a, output wire y);
    supply1 vdd;           // the supply rail: logic 1
    supply0 gnd;           // ground: logic 0

    nmos p1(y, vdd, a);    // pull-up: conducts when a is 0
    nmos n1(y, gnd, a);    // pull-down: conducts when a is 1
endmodule
