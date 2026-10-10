// Multiplexers and a decoder, built from gates.

// 2-to-1 multiplexer from four NANDs: y = s ? d1 : d0
module mux2(input wire d0, input wire d1, input wire s, output wire y);
    wire ns, t0, t1;
    nand g1(ns, s, s);         // ns = NOT s
    nand g2(t0, d0, ns);       // 0 only when d0 is chosen and is 1
    nand g3(t1, d1, s);        // 0 only when d1 is chosen and is 1
    nand g4(y, t0, t1);
endmodule

// 4-to-1 multiplexer from three 2-to-1 multiplexers: s = 2'b10 chooses d[2]
module mux4(input wire [3:0] d, input wire [1:0] s, output wire y);
    wire lo, hi;
    mux2 m0(.d0(d[0]), .d1(d[1]), .s(s[0]), .y(lo));
    mux2 m1(.d0(d[2]), .d1(d[3]), .s(s[0]), .y(hi));
    mux2 m2(.d0(lo),   .d1(hi),   .s(s[1]), .y(y));
endmodule

// 2-to-4 decoder with enable: exactly one output is 1 when en is 1
module dec2to4(input wire [1:0] s, input wire en, output wire [3:0] y);
    wire n0, n1;
    not g0(n0, s[0]);
    not g1(n1, s[1]);
    and g2(y[0], en, n1,   n0);
    and g3(y[1], en, n1,   s[0]);
    and g4(y[2], en, s[1], n0);
    and g5(y[3], en, s[1], s[0]);
endmodule
