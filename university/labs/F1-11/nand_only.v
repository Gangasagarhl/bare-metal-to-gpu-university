// Every gate built from two-input NAND gates only.
// "nand" is Verilog's built-in gate primitive: nand name(output, input1, input2).

module not_n(input wire a, output wire y);            // 1 NAND
    nand g1(y, a, a);
endmodule

module and_n(input wire a, input wire b, output wire y);   // 2 NANDs
    wire t;
    nand g1(t, a, b);
    nand g2(y, t, t);
endmodule

module or_n(input wire a, input wire b, output wire y);    // 3 NANDs
    wire na, nb;
    nand g1(na, a, a);
    nand g2(nb, b, b);
    nand g3(y, na, nb);
endmodule

module nor_n(input wire a, input wire b, output wire y);   // 4 NANDs
    wire t;
    or_n  u1(.a(a), .b(b), .y(t));
    not_n u2(.a(t), .y(y));
endmodule

module xor_n(input wire a, input wire b, output wire y);   // 4 NANDs
    wire t, u, v;
    nand g1(t, a, b);
    nand g2(u, a, t);
    nand g3(v, b, t);
    nand g4(y, u, v);
endmodule

module xnor_n(input wire a, input wire b, output wire y);  // 5 NANDs
    wire t;
    xor_n u1(.a(a), .b(b), .y(t));
    not_n u2(.a(t), .y(y));
endmodule
