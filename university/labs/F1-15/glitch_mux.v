// The four-NAND 2-to-1 multiplexer of F1-12, with 1 time unit of delay per NAND,
// and a "covered" version with one extra NAND that holds the output steady.
module mux2_d(input wire d0, input wire d1, input wire s, output wire y);
    wire ns, t0, t1;
    nand #1 g1(ns, s, s);
    nand #1 g2(t0, d0, ns);
    nand #1 g3(t1, d1, s);
    nand #1 g4(y, t0, t1);
endmodule

module mux2_covered(input wire d0, input wire d1, input wire s, output wire y);
    wire ns, t0, t1, t2;
    nand #1 g1(ns, s, s);
    nand #1 g2(t0, d0, ns);
    nand #1 g3(t1, d1, s);
    nand #1 g5(t2, d0, d1);    // extra term: 0 whenever d0 and d1 are both 1
    nand #1 g4(y, t0, t1, t2); // a three-input NAND
endmodule
