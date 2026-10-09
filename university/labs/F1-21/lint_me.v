// A module that simulates, but has two problems a careful reader (or a lint tool)
// should find. Used in the Code walk-through.
`timescale 1ns/1ns
module lint_me (
    input  wire       clk,
    input  wire       enable,     // problem 1: declared, never used
    input  wire [7:0] a,
    input  wire [7:0] b,
    output reg  [7:0] sum
);
    always @(posedge clk)
        sum <= a + b + 4'd10;     // problem 2: what if a + b + 10 is more than 255?
endmodule
