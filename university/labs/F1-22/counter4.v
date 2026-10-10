// A 4-bit synchronous counter: all four flip-flops share one clock.
// On a rising clock edge: reset wins, otherwise enable adds 1 (15 + 1 wraps to 0).
`timescale 1ns/1ns
module counter4 (
    input  wire       clk,
    input  wire       reset,
    input  wire       enable,
    output reg  [3:0] count,
    output wire       wrap      // 1 while count = 15 and enable = 1: the next edge wraps
);
    assign wrap = enable && (count == 4'd15);

    always @(posedge clk) begin
        if (reset)
            count <= 4'd0;
        else if (enable)
            count <= count + 4'd1;
    end
endmodule
