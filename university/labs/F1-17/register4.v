// A 4-bit register: four D flip-flops that share one clock.
// On a rising clock edge: reset wins, otherwise load copies d, otherwise q holds.
`timescale 1ns/1ns
module register4 (
    input  wire       clk,
    input  wire       reset,   // synchronous: acts only at a rising edge
    input  wire       load,
    input  wire [3:0] d,
    output reg  [3:0] q
);
    always @(posedge clk) begin
        if (reset)
            q <= 4'd0;
        else if (load)
            q <= d;
    end
endmodule
