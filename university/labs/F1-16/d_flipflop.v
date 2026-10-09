// A D flip-flop: q takes the value of d only at the rising edge of clk.
`timescale 1ns/1ns
module d_flipflop (
    input  wire clk,
    input  wire d,
    output reg  q
);
    always @(posedge clk)
        q <= d;
endmodule
