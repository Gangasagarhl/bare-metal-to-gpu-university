// Half adder and full adder, built from gates.

module half_adder(input wire a, input wire b, output wire s, output wire c);
    xor g1(s, a, b);           // sum bit: 1 when exactly one input is 1
    and g2(c, a, b);           // carry bit: 1 when both inputs are 1
endmodule

// A full adder from two half adders and an OR gate.
module full_adder(input wire a, input wire b, input wire cin,
                  output wire s, output wire cout);
    wire s1, c1, c2;
    half_adder h1(.a(a),  .b(b),   .s(s1), .c(c1));
    half_adder h2(.a(s1), .b(cin), .s(s),  .c(c2));
    or g3(cout, c1, c2);
endmodule

// The same full adder from nine NAND gates only.
module full_adder_nand(input wire a, input wire b, input wire cin,
                       output wire s, output wire cout);
    wire t1, t2, t3, x1, t4, t5, t6;
    nand g1(t1, a, b);         // g1..g4: x1 = a XOR b (the four-NAND XOR of F1-11)
    nand g2(t2, a, t1);
    nand g3(t3, b, t1);
    nand g4(x1, t2, t3);
    nand g5(t4, x1, cin);      // g5..g8: s = x1 XOR cin
    nand g6(t5, x1, t4);
    nand g7(t6, cin, t4);
    nand g8(s, t5, t6);
    nand g9(cout, t1, t4);     // cout = (a AND b) OR (x1 AND cin)
endmodule
