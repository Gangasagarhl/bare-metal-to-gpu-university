// Forensic evidence: Kofi's full adder, exactly as handed in.
module full_adder(input wire a, input wire b, input wire cin,
                  output wire s, output wire cout);
    wire s1, c1;
    xor g1(s1, a, b);
    xor g2(s, s1, cin);
    and g3(c1, a, b);
    assign cout = c1;          // carry out
endmodule

// Kofi's file did not include a NAND version; this copy keeps the testbench unchanged.
module full_adder_nand(input wire a, input wire b, input wire cin,
                       output wire s, output wire cout);
    assign {cout, s} = a + b + cin;
endmodule
