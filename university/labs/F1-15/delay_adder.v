// The 9-NAND full adder of F1-13, now with a delay: every NAND takes 1 time unit
// to change its output after an input changes ("#1"). The unit is our model's own,
// not nanoseconds of any real chip.
module full_adder_d(input wire a, input wire b, input wire cin,
                    output wire s, output wire cout);
    wire t1, t2, t3, x1, t4, t5, t6;
    nand #1 g1(t1, a, b);
    nand #1 g2(t2, a, t1);
    nand #1 g3(t3, b, t1);
    nand #1 g4(x1, t2, t3);
    nand #1 g5(t4, x1, cin);
    nand #1 g6(t5, x1, t4);
    nand #1 g7(t6, cin, t4);
    nand #1 g8(s, t5, t6);
    nand #1 g9(cout, t1, t4);
endmodule

// An N-bit ripple-carry adder made of those full adders.
module adder_d #(parameter N = 4)
    (input wire [N-1:0] a, input wire [N-1:0] b, input wire cin,
     output wire [N-1:0] s, output wire cout);
    wire [N:0] c;
    assign c[0] = cin;
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : bitslice
            full_adder_d fa(.a(a[i]), .b(b[i]), .cin(c[i]), .s(s[i]), .cout(c[i+1]));
        end
    endgenerate
    assign cout = c[N];
endmodule
