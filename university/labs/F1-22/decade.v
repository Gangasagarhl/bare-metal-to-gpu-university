// A decade counter (0, 1, ..., 9, 0, ...) for one digit of a clock display.
`timescale 1ns/1ns
module decade (
    input  wire       clk,
    input  wire       reset,
    output reg  [3:0] digit
);
    always @(posedge clk) begin
        if (reset || digit == 4'd9)
            digit <= 4'd0;
        else
            digit <= digit + 4'd1;
    end
endmodule
