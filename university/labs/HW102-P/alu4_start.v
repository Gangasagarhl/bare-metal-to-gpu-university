// HW102 practical exam (P) - STARTING FILE. Complete the module alu4 below.
//
// Rules (see the paper): structural only. You may use Verilog's gate primitives
// (and, or, xor, nand, nor, not), the module full_adder from full_adder.v (given),
// and the module mux2 below (given, from F1-12). "assign" may only join two wires
// (assign x = y;). No arithmetic operators (+ -), no always blocks, no ? : operator,
// no case statements. Build the result multiplexer from mux2 instances.
//
// Opcode table (the testbench tb_alu4_exam.v checks exactly this):
//   op  operation        op  operation
//   000 ADD  a + b       100 XOR  a ^ b
//   001 SUB  a - b       101 NOT  ~a
//   010 AND  a & b       110 PASS a
//   011 OR   a | b       111 PASS b
// Flags: zero = 1 when y is 0000; negative = y[3]; carry = carry out of bit 3 for
// ADD and SUB (0 for the other operations); overflow = (carry into bit 3) XOR
// (carry out of bit 3) for ADD and SUB (0 for the other operations).
//
// As handed out, this file compiles and the testbench prints FAIL: that is the
// expected starting state. Build and run with the course command:
//   iverilog -g2012 -Wall -o alu4_exam tb_alu4_exam.v full_adder.v alu4_start.v && vvp -n alu4_exam

// GIVEN: the four-NAND 2-to-1 multiplexer of F1-12. y = d0 when s = 0, d1 when s = 1.
module mux2(input wire d0, input wire d1, input wire s, output wire y);
    wire ns, t0, t1;
    nand g1(ns, s, s);
    nand g2(t0, d0, ns);
    nand g3(t1, d1, s);
    nand g4(y, t0, t1);
endmodule

// TODO 1: write a module mux8 (8 data inputs d[7:0], 3 select bits s[2:0], output y)
//         as a tree of seven mux2 instances. Which select bit goes to which level?

module alu4(input wire [3:0] a, input wire [3:0] b, input wire [2:0] op,
            output wire [3:0] y,
            output wire zero, output wire negative, output wire carry, output wire overflow);

    // TODO 2: decode the opcode with gates: sub = 1 only for op = 001;
    //         arith = 1 for op = 000 and op = 001.

    // TODO 3: prepare the adder's second operand: b_in[i] = b[i] XOR sub, and use
    //         sub as the adder's carry in (a - b = a + NOT b + 1).

    // TODO 4: chain four full_adder instances (full_adder.v); keep the carry wires
    //         c1, c2, c3 (carry into bits 1, 2, 3) and c4 (carry out of bit 3).

    // TODO 5: the logic operations, bit by bit: and, or, xor, not.

    // TODO 6: one mux8 per result bit, select = op, data inputs in opcode order.

    // TODO 7: the four flags from the definitions in the header.

    // Placeholders so that the file compiles as handed out. Replace them.
    assign y = a;
    assign zero = 1'b0;
    assign negative = 1'b0;
    assign carry = 1'b0;
    assign overflow = 1'b0;
endmodule
