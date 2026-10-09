// A full adder written as plain addition, for Yosys to map onto NAND (and NOT) cells.
module fa_behav(input wire a, input wire b, input wire cin, output wire s, output wire cout);
    assign {cout, s} = a + b + cin;
endmodule
