// A 4-bit ripple counter: each flip-flop toggles when the bit to its right falls
// from 1 to 0. Only bit 0 sees the real clock. Each flip-flop has a delay of 1 time unit.
`timescale 1ns/1ns
module ripple4 (
    input  wire       clk,
    input  wire       reset,     // asynchronous: clears at once
    output reg  [3:0] count
);
    always @(posedge clk or posedge reset)
        if (reset) count[0] <= 1'b0; else count[0] <= #1 ~count[0];
    always @(negedge count[0] or posedge reset)
        if (reset) count[1] <= 1'b0; else count[1] <= #1 ~count[1];
    always @(negedge count[1] or posedge reset)
        if (reset) count[2] <= 1'b0; else count[2] <= #1 ~count[2];
    always @(negedge count[2] or posedge reset)
        if (reset) count[3] <= 1'b0; else count[3] <= #1 ~count[3];
endmodule
