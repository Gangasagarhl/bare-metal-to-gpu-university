// The mux2 module on its own, so that mux4_bad.v can be tested with it.
module mux2(input wire d0, input wire d1, input wire s, output wire y);
    wire ns, t0, t1;
    nand g1(ns, s, s);
    nand g2(t0, d0, ns);
    nand g3(t1, d1, s);
    nand g4(y, t0, t1);
endmodule
