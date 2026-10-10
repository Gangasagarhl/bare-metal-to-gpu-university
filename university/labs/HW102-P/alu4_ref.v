// HW102 practical exam (P) - REFERENCE SOLUTION (Lab Engineer; not given to candidates).
// A structural 4-bit ALU: gate primitives, full_adder (full_adder.v) and mux2 only.
// Opcodes and flags as in alu4_start.v. Simulation only; untested on hardware.

module mux2(input wire d0, input wire d1, input wire s, output wire y);   // given (F1-12)
    wire ns, t0, t1;
    nand g1(ns, s, s);
    nand g2(t0, d0, ns);
    nand g3(t1, d1, s);
    nand g4(y, t0, t1);
endmodule

// TODO 1: an 8-to-1 multiplexer as a tree of seven mux2 (F1-12, Figure 2, one level more).
// The low select bit chooses within pairs, the middle bit between pairs, the top bit between halves.
module mux8(input wire [7:0] d, input wire [2:0] s, output wire y);
    wire m0, m1, m2, m3, n0, n1;
    mux2 l0(.d0(d[0]), .d1(d[1]), .s(s[0]), .y(m0));
    mux2 l1(.d0(d[2]), .d1(d[3]), .s(s[0]), .y(m1));
    mux2 l2(.d0(d[4]), .d1(d[5]), .s(s[0]), .y(m2));
    mux2 l3(.d0(d[6]), .d1(d[7]), .s(s[0]), .y(m3));
    mux2 k0(.d0(m0), .d1(m1), .s(s[1]), .y(n0));
    mux2 k1(.d0(m2), .d1(m3), .s(s[1]), .y(n1));
    mux2 top(.d0(n0), .d1(n1), .s(s[2]), .y(y));
endmodule

module alu4(input wire [3:0] a, input wire [3:0] b, input wire [2:0] op,
            output wire [3:0] y,
            output wire zero, output wire negative, output wire carry, output wire overflow);

    // TODO 2: opcode decode. arith = 1 for 000 and 001; sub = 1 for 001 only.
    wire nop2, nop1, arith, sub;
    not g_n2(nop2, op[2]);
    not g_n1(nop1, op[1]);
    and g_ar(arith, nop2, nop1);
    and g_sub(sub, arith, op[0]);

    // TODO 3: b_in = b XOR sub on every bit; the "+ 1" of two's complement enters as carry in.
    wire [3:0] b_in;
    xor gx0(b_in[0], b[0], sub);
    xor gx1(b_in[1], b[1], sub);
    xor gx2(b_in[2], b[2], sub);
    xor gx3(b_in[3], b[3], sub);

    // TODO 4: the ripple-carry adder (F1-14, Listing 1) with the carries kept.
    wire [3:0] sum;
    wire c1, c2, c3, c4;
    full_adder fa0(.a(a[0]), .b(b_in[0]), .cin(sub), .s(sum[0]), .cout(c1));
    full_adder fa1(.a(a[1]), .b(b_in[1]), .cin(c1),  .s(sum[1]), .cout(c2));
    full_adder fa2(.a(a[2]), .b(b_in[2]), .cin(c2),  .s(sum[2]), .cout(c3));
    full_adder fa3(.a(a[3]), .b(b_in[3]), .cin(c3),  .s(sum[3]), .cout(c4));

    // TODO 5: the logic operations, bit by bit.
    wire [3:0] y_and, y_or, y_xor, y_not;
    and ga0(y_and[0], a[0], b[0]);  and ga1(y_and[1], a[1], b[1]);
    and ga2(y_and[2], a[2], b[2]);  and ga3(y_and[3], a[3], b[3]);
    or  go0(y_or[0],  a[0], b[0]);  or  go1(y_or[1],  a[1], b[1]);
    or  go2(y_or[2],  a[2], b[2]);  or  go3(y_or[3],  a[3], b[3]);
    xor gx4(y_xor[0], a[0], b[0]);  xor gx5(y_xor[1], a[1], b[1]);
    xor gx6(y_xor[2], a[2], b[2]);  xor gx7(y_xor[3], a[3], b[3]);
    not gn0(y_not[0], a[0]);        not gn1(y_not[1], a[1]);
    not gn2(y_not[2], a[2]);        not gn3(y_not[3], a[3]);

    // TODO 6: the result multiplexer: one mux8 per bit, data inputs in opcode order
    //         d[0] ADD, d[1] SUB (both the adder's sum), d[2] AND, d[3] OR, d[4] XOR,
    //         d[5] NOT, d[6] PASS a, d[7] PASS b.
    mux8 mx0(.d({b[0], a[0], y_not[0], y_xor[0], y_or[0], y_and[0], sum[0], sum[0]}), .s(op), .y(y[0]));
    mux8 mx1(.d({b[1], a[1], y_not[1], y_xor[1], y_or[1], y_and[1], sum[1], sum[1]}), .s(op), .y(y[1]));
    mux8 mx2(.d({b[2], a[2], y_not[2], y_xor[2], y_or[2], y_and[2], sum[2], sum[2]}), .s(op), .y(y[2]));
    mux8 mx3(.d({b[3], a[3], y_not[3], y_xor[3], y_or[3], y_and[3], sum[3], sum[3]}), .s(op), .y(y[3]));

    // TODO 7: the flags.
    wire v_x;
    nor g_z(zero, y[0], y[1], y[2], y[3]);     // Z: no result bit is 1
    assign negative = y[3];                    // N: the top bit
    and g_c(carry, arith, c4);                 // C: carry out of bit 3, arithmetic only
    xor g_vx(v_x, c4, c3);                     // carry out XOR carry into the top bit
    and g_v(overflow, arith, v_x);             // V: arithmetic only
endmodule
