// A NAND with a typing mistake: the inside wire is spelled "m1" and never declared.
module nand2_typo(input wire a, input wire b, output wire y);
    supply1 vdd;
    supply0 gnd;
    pmos p1(y, vdd, a);
    pmos p2(y, vdd, b);
    nmos n1(y, m1, a);
    nmos n2(m1, gnd, b);
endmodule
