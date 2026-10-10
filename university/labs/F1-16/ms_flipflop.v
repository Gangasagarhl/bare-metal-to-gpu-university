// A master-slave D flip-flop built from two D latches.
// The master is open while clk = 0, the slave while clk = 1,
// so q can only change just after clk rises.
`timescale 1ns/1ns
module ms_flipflop (
    input  wire clk,
    input  wire d,
    output wire q
);
    wire m;   // the master latch's output
    d_latch master (.en(~clk), .d(d), .q(m));
    d_latch slave  (.en(clk),  .d(m), .q(q));
endmodule
