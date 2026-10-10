// Forensic evidence: the 4-bit adder from the "7 + 1" bug report, exactly as handed in.
module rca4(input wire [3:0] a, input wire [3:0] b, input wire cin,
            output wire [3:0] s, output wire cout);
    wire c1, c2, c3;                       // carries between the bits
    full_adder fa0(.a(a[0]), .b(b[0]), .cin(cin), .s(s[0]), .cout(c1));
    full_adder fa1(.a(a[1]), .b(b[1]), .cin(c1),  .s(s[1]), .cout(c2));
    full_adder fa2(.a(a[2]), .b(b[2]), .cin(c2),  .s(s[2]), .cout(c3));
    full_adder fa3(.a(a[3]), .b(b[3]), .cin(s[2]), .s(s[3]), .cout(cout));
endmodule
