// HW102 course project - REFERENCE SOLUTION (Lab Engineer, guide 11.5): a structural
// N-bit ALU built from NAND-based cells. Same opcodes and flags as the course ALU.
// Compile with -DTIMING to give every gate a delay of 1 model time unit (F1-15's model;
// the unit is not a nanosecond of any real chip). Simulation only; untested on hardware.
`ifdef TIMING
  `define D #1
`else
  `define D
`endif

// The nine-NAND full adder of F1-13.
module fa_g(input wire a, input wire b, input wire cin, output wire s, output wire cout);
    wire t1, t2, t3, x1, t4, t5, t6;
    nand `D g1(t1, a, b);
    nand `D g2(t2, a, t1);
    nand `D g3(t3, b, t1);
    nand `D g4(x1, t2, t3);
    nand `D g5(t4, x1, cin);
    nand `D g6(t5, x1, t4);
    nand `D g7(t6, cin, t4);
    nand `D g8(s, t5, t6);
    nand `D g9(cout, t1, t4);
endmodule

// The four-NAND 2-to-1 multiplexer of F1-12.
module mux2_g(input wire d0, input wire d1, input wire s, output wire y);
    wire ns, t0, t1;
    nand `D g1(ns, s, s);
    nand `D g2(t0, d0, ns);
    nand `D g3(t1, d1, s);
    nand `D g4(y, t0, t1);
endmodule

// 8-to-1 multiplexer: a tree of seven mux2_g; s[0] on the first level.
module mux8_g(input wire [7:0] d, input wire [2:0] s, output wire y);
    wire m0, m1, m2, m3, n0, n1;
    mux2_g l0(.d0(d[0]), .d1(d[1]), .s(s[0]), .y(m0));
    mux2_g l1(.d0(d[2]), .d1(d[3]), .s(s[0]), .y(m1));
    mux2_g l2(.d0(d[4]), .d1(d[5]), .s(s[0]), .y(m2));
    mux2_g l3(.d0(d[6]), .d1(d[7]), .s(s[0]), .y(m3));
    mux2_g k0(.d0(m0), .d1(m1), .s(s[1]), .y(n0));
    mux2_g k1(.d0(m2), .d1(m3), .s(s[1]), .y(n1));
    mux2_g top(.d0(n0), .d1(n1), .s(s[2]), .y(y));
endmodule

// The N-bit ALU (N a multiple of 4).
module alu_n #(parameter N = 16)
    (input wire [N-1:0] a, input wire [N-1:0] b, input wire [2:0] op,
     output wire [N-1:0] y,
     output wire zero, output wire negative, output wire carry, output wire overflow);

    // opcode decode: arith for 000 and 001, sub for 001
    wire nop2, nop1, arith, sub;
    not `D g_n2(nop2, op[2]);
    not `D g_n1(nop1, op[1]);
    and `D g_ar(arith, nop2, nop1);
    and `D g_sub(sub, arith, op[0]);

    // one bit slice per position: b XOR sub, full adder, logic gates, result mux
    wire [N:0] c;                              // c[i] is the carry into bit i
    assign c[0] = sub;
    wire [N-1:0] b_in, sum, y_and, y_or, y_xor, y_not;
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : slice
            xor `D gx(b_in[i], b[i], sub);
            fa_g fa(.a(a[i]), .b(b_in[i]), .cin(c[i]), .s(sum[i]), .cout(c[i+1]));
            and `D ga(y_and[i], a[i], b[i]);
            or  `D go(y_or[i],  a[i], b[i]);
            xor `D gq(y_xor[i], a[i], b[i]);
            not `D gn(y_not[i], a[i]);
            mux8_g mx(.d({b[i], a[i], y_not[i], y_xor[i], y_or[i], y_and[i], sum[i], sum[i]}),
                      .s(op), .y(y[i]));
        end
    endgenerate

    // flags. Z: OR of each group of four result bits, then NOR of the groups.
    wire [N/4-1:0] grp;
    generate
        for (i = 0; i < N/4; i = i + 1) begin : zgroup
            or `D gz(grp[i], y[4*i], y[4*i+1], y[4*i+2], y[4*i+3]);
        end
    endgenerate
    wire [N/4:0] acc;                          // acc[j] = OR of groups 0 .. j-1
    assign acc[0] = 1'b0;
    generate
        for (i = 0; i < N/4; i = i + 1) begin : zchain
            or `D ga(acc[i+1], acc[i], grp[i]);
        end
    endgenerate
    not `D g_z(zero, acc[N/4]);
    assign negative = y[N-1];
    and `D g_c(carry, arith, c[N]);
    wire v_x;
    xor `D g_vx(v_x, c[N], c[N-1]);
    and `D g_v(overflow, arith, v_x);
endmodule
