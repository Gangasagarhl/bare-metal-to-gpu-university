// A 4-bit shift register: on each rising edge every bit moves one place to the left
// and serial_in enters on the right.
`timescale 1ns/1ns
module shift4 (
    input  wire       clk,
    input  wire       serial_in,
    output reg  [3:0] q
);
    always @(posedge clk)
        q <= {q[2:0], serial_in};
endmodule
