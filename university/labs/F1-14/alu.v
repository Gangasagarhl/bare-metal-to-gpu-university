// An N-bit ALU: a ripple-carry adder for ADD and SUB, gates for the logic operations,
// a multiplexer to choose the result, and four status flags.
//
// op  operation        op  operation
// 000 ADD  a + b       100 XOR  a ^ b
// 001 SUB  a - b       101 NOT  ~a
// 010 AND  a & b       110 PASS a
// 011 OR   a | b       111 PASS b

module adder_n #(parameter N = 4)
    (input wire [N-1:0] a, input wire [N-1:0] b, input wire cin,
     output wire [N-1:0] s, output wire cout, output wire c_msb);
    wire [N:0] c;                          // c[i] is the carry into bit i
    assign c[0] = cin;
    genvar i;
    generate
        for (i = 0; i < N; i = i + 1) begin : bitslice
            full_adder fa(.a(a[i]), .b(b[i]), .cin(c[i]), .s(s[i]), .cout(c[i+1]));
        end
    endgenerate
    assign cout = c[N];                    // carry out of the top bit
    assign c_msb = c[N-1];                 // carry into the top bit (for overflow)
endmodule

module alu #(parameter N = 4)
    (input wire [N-1:0] a, input wire [N-1:0] b, input wire [2:0] op,
     output reg [N-1:0] y,
     output wire zero, output wire negative, output wire carry, output wire overflow);
    wire sub = (op == 3'b001);
    wire [N-1:0] b_in = b ^ {N{sub}};      // SUB: invert every bit of b ...
    wire [N-1:0] sum;
    wire c_out, c_msb;
    adder_n #(N) add(.a(a), .b(b_in), .cin(sub), .s(sum), .cout(c_out), .c_msb(c_msb));
                                           // ... and add 1 through the carry-in
    always @* begin                        // the result multiplexer
        case (op)
            3'b000, 3'b001: y = sum;
            3'b010: y = a & b;
            3'b011: y = a | b;
            3'b100: y = a ^ b;
            3'b101: y = ~a;
            3'b110: y = a;
            default: y = b;
        endcase
    end

    wire arith = (op == 3'b000) | (op == 3'b001);
    assign zero     = ~|y;                 // NOR of all result bits
    assign negative = y[N-1];              // the top bit (two's complement sign)
    assign carry    = arith & c_out;
    assign overflow = arith & (c_out ^ c_msb);
endmodule
